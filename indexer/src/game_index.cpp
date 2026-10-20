#include "game_index.hpp"

#include <algorithm>
#include <exception>
#include <format>

#include <bitboard/intrinsics.hpp>
#include <core/chessboard.hpp>
#include <elephant_gambit.hpp>
#include <io/fen_parser.hpp>
#include <io/san_parser.hpp>
#include <move/generation/tusk/move_generator.hpp>
#include <move/move.hpp>
#include <move/move_executor.hpp>

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
    return "none";
}

GameIndex index(const io::pgn_parser::Game& game, u32 number, bool verbose)
{
    GameIndex entry;
    entry.number = number;
    entry.offset = game.offset;
    entry.length = game.length;
    entry.tags = game.tags;

    std::string_view resultToken;
    auto fail = [&](std::string reason) {
        entry.ok = false;
        entry.error = std::move(reason);
    };

    Chessboard board;
    std::vector<u64> hashes;
    try {
        if (const std::string* fen = game.tag("FEN")) {
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
    io::pgn_parser::MovetextTokenizer tokenizer(game.movetext);
    while (std::optional<io::pgn_parser::Token> token = tokenizer.next()) {
        if (token->kind == io::pgn_parser::Token::Kind::Result) {
            resultToken = token->text;
            continue;
        }
        if (token->kind != io::pgn_parser::Token::Kind::Move || !entry.ok)
            continue;

        const Set us = board.readToPlay();
        PackedMove move;
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
        hashes.push_back(board.readPosition().hash());

        ++entry.plies;
        if (move.isCapture())
            ++entry.captures;
        if (move.isPromotion())
            ++entry.promotions[static_cast<size_t>(static_cast<i32>(PieceType::QUEEN) - move.readPromoteToPieceType())];
        if (move.isCastling())
            (us == Set::WHITE ? entry.castledWhite : entry.castledBlack) = entry.plies;
        if (std::find(c_materialPlies.begin(), c_materialPlies.end(), entry.plies) != c_materialPlies.end())
            entry.material.emplace_back(std::to_string(entry.plies), material(board.readPosition()));
        if (verbose)
            entry.moves.push_back(uci(move));
    }

    const std::string* resultTag = game.tag("Result");
    entry.result = resultTag ? *resultTag : resultToken.empty() ? "*" : std::string(resultToken);

    if (entry.ok) {
        entry.material.emplace_back("end", material(board.readPosition()));
        entry.ending = ending(board, hashes);
    }
    if (verbose)
        io::fen_parser::serialize(board, entry.fen);
    return entry;
}

std::string toJson(const GameIndex& entry, bool verbose)
{
    std::string out;
    out.reserve(512);
    out += "{\"indexer\":";
    appendEscaped(out, c_formatVersion);
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
        appendEscaped(out, toString(entry.ending));
        out += std::format(",\"promotions\":{{\"q\":{},\"r\":{},\"b\":{},\"n\":{}}}", entry.promotions[0],
            entry.promotions[1], entry.promotions[2], entry.promotions[3]);
        out += std::format(",\"castled\":{{\"white\":{},\"black\":{}}}", ply(entry.castledWhite), ply(entry.castledBlack));
        out += std::format(",\"captures\":{},\"material\":{{", entry.captures);
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
