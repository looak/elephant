#include "game_index.hpp"
#include "polyglot.hpp"

#include <algorithm>
#include <exception>
#include <format>

#include <bitboard/intrinsics.hpp>
#include <core/build_info.hpp>
#include <core/chessboard.hpp>
#include <elephant_gambit.hpp>
#include <io/fen_parser.hpp>
#include <io/san_parser.hpp>
#include <move/generation/tusk/move_generator.hpp>
#include <move/move.hpp>
#include <move/move_executor.hpp>
#include <position/hash_zobrist.hpp>

// a move that doesn't read is only told apart from one that does by the parsers' exceptions.
#ifndef EXCEPTIONS_ENABLED
#error "phant-indexer needs EXCEPTIONS_ENABLED, the SAN & PGN parsers report an illegal move by throwing"
#endif

namespace indexer {
namespace {
    constexpr u64 c_lightSquares = 0x55AA55AA55AA55AAull;
    constexpr u64 c_darkSquares = ~c_lightSquares;

    u64 pieces(PositionReader position, Set set, u8 pieceId) { return position.material().read(set, pieceId).read(); }

    i32 count(PositionReader position, Set set, u8 pieceId) { return intrinsics::popcnt(pieces(position, set, pieceId)); }

    u32 pieceCount(PositionReader position)
    {
        return static_cast<u32>(intrinsics::popcnt(position.material().combine().read()));
    }

    template<Set us>
    bool checked(PositionReader position) { return tusk::CheckInfo<us>(position).isChecked(); }

    bool inCheck(const Chessboard& board)
    {
        return board.readToPlay() == Set::WHITE ? checked<Set::WHITE>(board.readPosition())
                                                : checked<Set::BLACK>(board.readPosition());
    }

    // a side passing, as python-chess plays a null move: no en passant capture for the other side, the fifty move
    // count goes on.
    void makeNullMove(Chessboard& board)
    {
        PositionEditor position = board.editPosition();
        position.enPassant().clear();
        position.hash() = zobrist::updateBlackToMoveHash(position.hash());
        GameState& state = board.editState();
        ++state.plyCount;
        state.whiteToMove = !state.whiteToMove;
        state.moveCount += static_cast<u16>(state.whiteToMove);
    }

    std::string readerVersion()
    {
        for (const auto& [key, value] : build_info::entries()) {
            if (key == "version")
                return value;
        }
        return "unknown";
    }

    i32 material(PositionReader position)
    {
        auto side = [&](Set set) {
            return count(position, set, pawnId) + 3 * count(position, set, knightId) + 3 * count(position, set, bishopId)
                + 5 * count(position, set, rookId) + 9 * count(position, set, queenId);
        };
        return side(Set::WHITE) - side(Set::BLACK);
    }

    // neither side can mate whatever is played: no pawn, rook or queen, and at most one minor piece or bishops
    // all on squares of one colour.
    bool insufficientMaterial(PositionReader position)
    {
        u64 heavy = 0, knights = 0, bishops = 0;
        for (Set set : { Set::WHITE, Set::BLACK }) {
            heavy |= pieces(position, set, pawnId) | pieces(position, set, rookId) | pieces(position, set, queenId);
            knights |= pieces(position, set, knightId);
            bishops |= pieces(position, set, bishopId);
        }
        if (heavy)
            return false;
        if (intrinsics::popcnt(knights | bishops) <= 1)
            return true;
        return knights == 0 && ((bishops & c_lightSquares) == 0 || (bishops & c_darkSquares) == 0);
    }

    template<Set us>
    std::pair<bool, bool> legalMoveAndCheck(PositionReader position)
    {
        tusk::CheckInfo<us> checkInfo(position);
        tusk::MoveGenerator<us> gen(position, checkInfo);
        tusk::MoveGenResult<us> moves = gen.generateAll();
        return { static_cast<bool>(moves.next()), checkInfo.isChecked() };
    }

    Ending ending(const Chessboard& board, const std::vector<u64>& hashes)
    {
        const PositionReader position = board.readPosition();
        const auto [canMove, checked] = board.readToPlay() == Set::WHITE ? legalMoveAndCheck<Set::WHITE>(position)
                                                                          : legalMoveAndCheck<Set::BLACK>(position);
        if (!canMove)
            return checked ? Ending::Checkmate : Ending::Stalemate;
        if (insufficientMaterial(position))
            return Ending::InsufficientMaterial;
        if (board.readState().plyCount >= 100)
            return Ending::FiftyMoves;
        if (std::count(hashes.begin(), hashes.end(), hashes.back()) >= 3)
            return Ending::Repetition;
        return Ending::None;
    }

    std::string uci(PackedMove move)
    {
        if (move == PackedMove::NullMove())
            return "0000";
        auto square = [](Square sqr) {
            const i32 index = static_cast<i32>(sqr);
            return std::string{ static_cast<char>('a' + index % 8), static_cast<char>('1' + index / 8) };
        };
        std::string text = square(move.sourceSqr()) + square(move.targetSqr());
        if (move.isPromotion())
            text += "nbrq"[move.readPromoteToPieceType() - static_cast<i32>(PieceType::KNIGHT)];
        return text;
    }

    void appendEscaped(std::string& out, std::string_view text)
    {
        out += '"';
        for (char c : text) {
            switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                    out += std::format("\\u{:04x}", static_cast<unsigned>(c));
                else
                    out += c;
            }
        }
        out += '"';
    }
} // namespace

std::string_view toString(Ending ending)
{
    switch (ending) {
    case Ending::Checkmate: return "checkmate";
    case Ending::Stalemate: return "stalemate";
    case Ending::InsufficientMaterial: return "insufficient_material";
    case Ending::FiftyMoves: return "fifty_moves";
    case Ending::Repetition: return "repetition";
    case Ending::None: break;
    }
    return "null";
}

PositionFacts positionFacts(const Chessboard& board)
{
    const PositionReader position = board.readPosition();
    const auto [canMove, checked] = board.readToPlay() == Set::WHITE ? legalMoveAndCheck<Set::WHITE>(position)
                                                                      : legalMoveAndCheck<Set::BLACK>(position);
    return { checked, canMove };
}

GameIndex index(const io::pgn_parser::Game& game, u32 number, bool verbose, bool positions, bool moves)
{
    static const std::string c_reader = readerVersion();
    GameIndex entry;
    entry.reader = c_reader;
    entry.number = number;
    entry.offset = game.offset;
    entry.length = game.length;
    for (const io::pgn_parser::Tag& tag : game.tags) {
        auto same = std::find_if(entry.tags.begin(), entry.tags.end(), [&](const auto& kept) { return kept.name == tag.name; });
        if (same == entry.tags.end())
            entry.tags.push_back(tag);
        else
            same->value = tag.value;
    }
    auto tag = [&](std::string_view name) -> const std::string* {
        auto found = std::find_if(entry.tags.begin(), entry.tags.end(), [&](const auto& kept) { return kept.name == name; });
        return found == entry.tags.end() ? nullptr : &found->value;
    };

    // python-chess's: the Result tag, unless it's * or missing, then the first result in the movetext, while it reads.
    const std::string* resultTag = tag("Result");
    entry.result = resultTag ? *resultTag : "*";
    auto fail = [&](std::string reason) {
        entry.ok = false;
        entry.error = std::move(reason);
    };

    Chessboard board;
    std::vector<u64> hashes;
    try {
        const std::string* variant = tag("Variant");
        if (variant && !io::pgn_parser::standardVariant(*variant))
            fail(std::format("not standard chess: Variant {}", *variant));
        else if (const std::string* fen = tag("FEN")) {
            if (!io::fen_parser::deserialize(*fen, board))
                fail(std::format("the FEN tag doesn't read: {}", *fen));
        }
        else {
            chess::NewGame(board);
        }
    }
    catch (const std::exception& e) {
        fail(std::format("the FEN tag doesn't read: {}", e.what()));
    }

    MoveUndoUnit undo;
    hashes.push_back(board.readPosition().hash());
    entry.minPieces = pieceCount(board.readPosition());
    entry.start = polyglotHash(board);
    if (moves)
        io::fen_parser::serialize(board, entry.startFen);
    if (positions)
        entry.positions.push_back(entry.start);
    io::pgn_parser::MovetextTokenizer tokenizer(game.movetext);
    while (std::optional<io::pgn_parser::Token> token = tokenizer.next()) {
        using Kind = io::pgn_parser::Token::Kind;
        if (!entry.ok)
            break;  // python-chess reads nothing of the game after an error
        if (token->kind == Kind::Result) {
            if (entry.result == "*")
                entry.result = token->text;
            continue;
        }
        if (token->kind != Kind::Move && token->kind != Kind::NullMove)
            continue;

        const Set us = board.readToPlay();
        PackedMove move = PackedMove::NullMove();
        if (token->kind == Kind::NullMove) {
            makeNullMove(board);
        }
        else {
            try {
                move = io::san_parser::deserialize(board.readPosition(), us == Set::WHITE, token->text);
            }
            catch (const std::exception& e) {
                entry.errorPly = entry.plies + 1;
                fail(std::format("{}: {}", token->text, e.what()));
                continue;
            }

            MoveExecutor executor(board.editPosition());
            GameState& state = board.editState();
            executor.makeMove<true>(move, undo, state.plyCount);
            state.whiteToMove = !state.whiteToMove;
            state.moveCount += static_cast<u16>(state.whiteToMove);
        }
        hashes.push_back(board.readPosition().hash());
        if (moves)
            entry.played.push_back(move);

        ++entry.plies;
        if (positions || entry.plies == c_openingPly) {
            const u64 position = polyglotHash(board);
            if (positions)
                entry.positions.push_back(position);
            if (entry.plies == c_openingPly)
                entry.opening = position;
        }
        if (move.isCapture()) {
            ++entry.captures;
            entry.minPieces = std::min(entry.minPieces, pieceCount(board.readPosition()));
        }
        if (move.isEnPassant())
            ++entry.epCaptures;
        if (token->kind == Kind::Move && inCheck(board))
            ++entry.checks;
        if (move.isPromotion())
            ++entry.promotions[static_cast<size_t>(static_cast<i32>(PieceType::QUEEN) - move.readPromoteToPieceType())];
        if (move.isCastling())
            (us == Set::WHITE ? entry.castledWhite : entry.castledBlack) = entry.plies;
        if (std::find(c_materialPlies.begin(), c_materialPlies.end(), entry.plies) != c_materialPlies.end())
            entry.material.emplace_back(std::to_string(entry.plies), material(board.readPosition()));
        if (verbose)
            entry.moves.push_back(uci(move));
    }


    if (entry.ok) {
        entry.material.emplace_back("end", material(board.readPosition()));
        entry.ending = ending(board, hashes);
    }
    if (verbose)
        io::fen_parser::serialize(board, entry.fen);
    return entry;
}

std::string toJsonString(std::string_view text)
{
    std::string out;
    appendEscaped(out, text);
    return out;
}

std::string toJson(const GameIndex& entry, bool verbose)
{
    std::string out;
    out.reserve(512);
    out += std::format("{{\"format\":{},\"reader\":", c_formatVersion);
    appendEscaped(out, entry.reader);
    if (!entry.source.empty()) {
        out += ",\"source\":";
        appendEscaped(out, entry.source);
    }
    out += std::format(",\"game\":{},\"offset\":{},\"length\":{},\"ok\":{}", entry.number, entry.offset, entry.length,
        entry.ok ? "true" : "false");

    out += ",\"tags\":{";
    for (size_t i = 0; i < entry.tags.size(); ++i) {
        if (i)
            out += ',';
        appendEscaped(out, entry.tags[i].name);
        out += ':';
        appendEscaped(out, entry.tags[i].value);
    }
    out += "},\"result\":";
    appendEscaped(out, entry.result);
    out += std::format(",\"plies\":{}", entry.plies);

    if (!entry.ok) {
        out += std::format(",\"error\":{{\"ply\":{},\"reason\":", entry.errorPly);
        appendEscaped(out, entry.error);
        out += '}';
    }
    else {
        auto ply = [](const std::optional<u32>& value) { return value ? std::to_string(*value) : std::string("null"); };
        out += ",\"ended\":";
        if (entry.ending == Ending::None)
            out += "null";
        else
            appendEscaped(out, toString(entry.ending));
        out += std::format(",\"start\":\"{}\",\"opening\":", toHex(entry.start));
        out += entry.opening ? std::format("\"{}\"", toHex(*entry.opening)) : std::string("null");
        out += std::format(",\"promotions\":{{\"q\":{},\"r\":{},\"b\":{},\"n\":{}}}", entry.promotions[0],
            entry.promotions[1], entry.promotions[2], entry.promotions[3]);
        out += std::format(",\"castled\":{{\"white\":{},\"black\":{}}}", ply(entry.castledWhite), ply(entry.castledBlack));
        out += std::format(",\"captures\":{},\"ep_captures\":{},\"checks\":{},\"min_pieces\":{},\"material\":{{",
            entry.captures, entry.epCaptures, entry.checks, entry.minPieces);
        for (size_t i = 0; i < entry.material.size(); ++i)
            out += std::format("{}\"{}\":{}", i ? "," : "", entry.material[i].first, entry.material[i].second);
        out += '}';
    }

    if (verbose) {
        out += ",\"moves\":[";
        for (size_t i = 0; i < entry.moves.size(); ++i) {
            if (i)
                out += ',';
            appendEscaped(out, entry.moves[i]);
        }
        out += "],\"fen\":";
        appendEscaped(out, entry.fen);
    }
    out += '}';
    return out;
}

} // namespace indexer
