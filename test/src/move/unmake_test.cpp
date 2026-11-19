#include <gtest/gtest.h>
#include <string_view>
#include <utility>
#include <vector>

#include <material/chess_piece.hpp>
#include <core/chessboard.hpp>
#include <core/game.hpp>
#include <move/move.hpp>
#include <move_gen_ab.hpp>
#include <move/move_executor.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class UnmakeFixture : public ::testing::TestWithParam<Generator> {
public:
    virtual void SetUp() {

    };
    virtual void TearDown() {};

    void setup(std::string_view fen)
    {
        ASSERT_TRUE(m_game.loadFen(fen)) << fen;
    }

    // the moves of the side to move as the generator builds them, with their capture, castling, en passant & promotion
    // flags.
    std::vector<PackedMove> legalMoves()
    {
        std::vector<PackedMove> moves;
        auto add = [&](PackedMove mv) { moves.push_back(mv); };
        if (m_game.readToPlay() == Set::WHITE)
            forEachLegalMove<Set::WHITE>(GetParam(), m_game.readPosition(), add);
        else
            forEachLegalMove<Set::BLACK>(GetParam(), m_game.readPosition(), add);
        return moves;
    }

    PackedMove findMove(Square source, Square target, PieceType promoteTo = PieceType::NONE)
    {
        for (PackedMove mv : legalMoves()) {
            if (mv.sourceSqr() != source || mv.targetSqr() != target)
                continue;
            if (promoteTo == PieceType::NONE ? !mv.isPromotion()
                : mv.isPromotion() && mv.readPromoteToPieceType() == static_cast<i32>(promoteTo))
                return mv;
        }
        ADD_FAILURE() << "no legal move from " << source << " to " << target;
        return PackedMove::NullMove();
    }

    // makes the move and returns the board as it was before it.
    Chessboard makeMove(PackedMove move)
    {
        Chessboard before = m_game.board();
        m_game.makeMove<true>(move);
        return before;
    }

    // the board, hash, castling, en passant, side to move & move counters are all back to expected.
    void expectState(const Chessboard& expected, PackedMove unmade)
    {
        const Chessboard& board = m_game.board();
        PositionReader actual = board.readPosition();
        PositionReader wanted = expected.readPosition();
        const std::string context = "after unmaking " + unmade.toString();
        for (byte sqr = 0; sqr < 64; ++sqr) {
            const Square square = static_cast<Square>(sqr);
            EXPECT_EQ(wanted.pieceAt(square), actual.pieceAt(square)) << "square " << square << " " << context;
        }
        EXPECT_EQ(wanted.hash(), actual.hash()) << context;
        EXPECT_EQ(static_cast<int>(wanted.castling().read()), static_cast<int>(actual.castling().read())) << context;
        EXPECT_EQ(static_cast<int>(wanted.enPassant().read()), static_cast<int>(actual.enPassant().read())) << context;
        EXPECT_EQ(expected.readToPlay(), board.readToPlay()) << context;
        EXPECT_EQ(expected.readHalfmoveClock(), board.readHalfmoveClock()) << context;
        EXPECT_EQ(expected.readMoveCount(), board.readMoveCount()) << context;
        EXPECT_TRUE(board.compare(expected)) << context;
    }

    Game m_game;
};
////////////////////////////////////////////////////////////////

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][ P ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// 1. e3, unmake, e4, unmake, then with a black pawn on d4: e4, unmake.
TEST_P(UnmakeFixture, Pawn_SimpleMoves)
{
    // setup
    Chessboard board;
    board.editPosition().placePiece(piece_constants::white_pawn, Square::E2);
    m_game = Game(board);
    PositionReader position = m_game.readPosition();
    u64 orgHash = position.hash();

    
    PackedMove move(Square::E2, Square::E3);

    // do e3 move
    m_game.makeMove<true>(move);

    // verify
    EXPECT_NE(orgHash, position.hash()); 
    EXPECT_FALSE(position.enPassant());
    EXPECT_EQ(piece_constants::white_pawn, position.pieceAt(Square::E3));
    EXPECT_EQ(ChessPiece::None(), position.pieceAt(Square::E2));

    // unmake move
    bool unmakeResult = m_game.unmakeMove();

    // verify state of board
    EXPECT_TRUE(unmakeResult);
    EXPECT_EQ(ChessPiece::None(), position.pieceAt(Square::E3));
    EXPECT_EQ(piece_constants::white_pawn, position.pieceAt(Square::E2));
    EXPECT_FALSE(position.enPassant());
    EXPECT_EQ(orgHash, position.hash());

    PackedMove moveE4(Square::E2, Square::E4);

    // do e4 move, no black pawn stands beside e4 so no en passant square is recorded (or hashed).
    m_game.makeMove<true>(moveE4);
    EXPECT_FALSE(position.enPassant());
    EXPECT_TRUE(m_game.unmakeMove());
    EXPECT_EQ(orgHash, position.hash());

    // with a black pawn on d4 able to capture en passant, e4 records e3.
    board.editPosition().placePiece(piece_constants::black_pawn, Square::D4);
    m_game = Game(board);
    orgHash = position.hash();
    m_game.makeMove<true>(moveE4);

    // verify
    EXPECT_EQ(Square::E3, position.enPassant().readSquare());
    EXPECT_EQ(piece_constants::white_pawn, position.pieceAt(Square::E4));
    EXPECT_EQ(ChessPiece::None(), position.pieceAt(Square::E2));
    EXPECT_NE(orgHash, position.hash());

    // unmake move
    unmakeResult = m_game.unmakeMove();

    // verify state of board
    EXPECT_TRUE(unmakeResult);
    EXPECT_EQ(ChessPiece::None(), position.pieceAt(Square::E4));
    EXPECT_EQ(piece_constants::white_pawn, position.pieceAt(Square::E2));
    EXPECT_FALSE(position.enPassant());
    EXPECT_EQ(orgHash, position.hash());

}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][ p ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][ P ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// Moves:
// 1. e4 dxe3 e.p.
TEST_P(UnmakeFixture, EnPassant_Captured_Unmake)
{
    // setup    
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;
    Chessboard board;
    board.editPosition().placePieces(P, Square::E2, p, Square::D4);
    m_game = Game(board);
    PositionReader position = m_game.readPosition();


    // sanity check that the material is on the board
    const auto& material = position.material();
    EXPECT_EQ(1, material.whitePawns().count());
    EXPECT_EQ(1, material.blackPawns().count());

    // move white pawn to e4
    PackedMove move{}; // zeroed, a default PackedMove is uninitialized and its flags are garbage
    move.setSource(Square::E2);
    move.setTarget(Square::E4);
    m_game.makeMove<true>(move);
  
    /**
     * En passant is a special pawn capture move in chess where a pawn captures an opposing pawn
     * that has just advanced two squares from its starting position, as if it had only advanced
     * one square. The capturing pawn moves diagonally to the square that the opposing pawn passed
     * over, and the captured pawn is removed from the board.     */
    EXPECT_TRUE(position.enPassant());
    EXPECT_EQ(Square::E3, position.enPassant().readSquare());
    EXPECT_EQ(P, position.pieceAt(Square::E4));
    EXPECT_EQ(p, position.pieceAt(Square::D4));
    EXPECT_EQ(1, material.whitePawns().count());
    EXPECT_EQ(1, material.blackPawns().count());

    // setup ep capture move
    PackedMove epCapture(Square::D4, Square::E3);
    epCapture.setCapture(true);
    epCapture.setEnPassant(true);

    // do
    m_game.makeMove<true>(epCapture);

    // validate
    EXPECT_EQ(Square::NullSQ, position.enPassant().readSquare());
    EXPECT_FALSE(position.enPassant());

    ChessPiece expected;  // default, "empty" piece
    EXPECT_EQ(expected, position.pieceAt(Square::E4));
    EXPECT_EQ(expected, position.pieceAt(Square::D4));
    EXPECT_EQ(p, position.pieceAt(Square::E3));

    EXPECT_EQ(0, material.whitePawns().count());
    EXPECT_EQ(1, material.blackPawns().count());

    // do
    bool result = m_game.unmakeMove();

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(Square::E3, position.enPassant().readSquare());
    EXPECT_EQ(P, position.pieceAt(Square::E4));
    EXPECT_EQ(p, position.pieceAt(Square::D4));
    EXPECT_EQ(expected, position.pieceAt(Square::E3));

    EXPECT_EQ(1, material.whitePawns().count());
    EXPECT_EQ(1, material.blackPawns().count());
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][ p ][   ][   ][ p ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][ P ][   ][   ][ P ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// every white move and every black reply, c4 & f4 give black an en passant capture each.
TEST_P(UnmakeFixture, UnmakeEnPassantMoves_VariousPositions_CorrectUndo)
{
    // setup
    setup("4k3/8/8/8/3p2p1/8/2P2P2/4K3 w - - 0 1");
    const Chessboard start = m_game.board();

    // do
    int enPassantCaptures = 0;
    for (PackedMove whiteMove : legalMoves()) {
        makeMove(whiteMove);
        const Chessboard afterWhite = m_game.board();

        for (PackedMove blackMove : legalMoves()) {
            enPassantCaptures += blackMove.isEnPassant() ? 1 : 0;
            makeMove(blackMove);
            EXPECT_TRUE(m_game.unmakeMove());
            expectState(afterWhite, blackMove);
        }

        EXPECT_TRUE(m_game.unmakeMove());
        expectState(start, whiteMove);
    }

    // validate
    EXPECT_EQ(2, enPassantCaptures);

    const auto& material = m_game.readPosition().material();
    EXPECT_EQ(2, material.whitePawns().count());
    EXPECT_TRUE(material.whitePawns()[Square::C2]);
    EXPECT_TRUE(material.whitePawns()[Square::F2]);
    EXPECT_EQ(2, material.blackPawns().count());
    EXPECT_TRUE(material.blackPawns()[Square::D4]);
    EXPECT_TRUE(material.blackPawns()[Square::G4]);
}

// 8 [   ][   ][   ][ n ][   ][   ][   ][ k ]
// 7 [   ][   ][   ][   ][ P ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// Moves:
// e8=Q, unmake, exd8=Q, unmake, exd8=N, unmake
TEST_P(UnmakeFixture, Pawn_Promotion_Unmake)
{
    auto P = WHITEPAWN;
    auto n = BLACKKNIGHT;
    setup("3n3k/4P3/8/8/8/8/8/4K3 w - - 0 1");
    const Chessboard start = m_game.board();
    const auto& material = m_game.readPosition().material();

    // e8=Q
    PackedMove promote = findMove(Square::E7, Square::E8, PieceType::QUEEN);
    EXPECT_TRUE(promote.isPromotion());
    EXPECT_FALSE(promote.isCapture());
    makeMove(promote);

    EXPECT_NE(start.readPosition().hash(), m_game.readPosition().hash());
    EXPECT_EQ(WHITEQUEEN, m_game.readPosition().pieceAt(Square::E8));
    EXPECT_EQ(ChessPiece::None(), m_game.readPosition().pieceAt(Square::E7));
    EXPECT_EQ(0, material.whitePawns().count());
    EXPECT_EQ(1, material.whiteQueens().count());

    EXPECT_TRUE(m_game.unmakeMove());
    expectState(start, promote);
    EXPECT_EQ(P, m_game.readPosition().pieceAt(Square::E7));
    EXPECT_EQ(1, material.whitePawns().count());
    EXPECT_EQ(0, material.whiteQueens().count());

    // exd8=Q & exd8=N
    for (PieceType promoteTo : { PieceType::QUEEN, PieceType::KNIGHT }) {
        PackedMove capturePromote = findMove(Square::E7, Square::D8, promoteTo);
        EXPECT_TRUE(capturePromote.isPromotion());
        EXPECT_TRUE(capturePromote.isCapture());
        makeMove(capturePromote);

        EXPECT_EQ(ChessPiece(Set::WHITE, promoteTo), m_game.readPosition().pieceAt(Square::D8));
        EXPECT_EQ(ChessPiece::None(), m_game.readPosition().pieceAt(Square::E7));
        EXPECT_EQ(0, material.blackKnights().count());
        EXPECT_EQ(0, material.whitePawns().count());

        EXPECT_TRUE(m_game.unmakeMove());
        expectState(start, capturePromote);
        EXPECT_EQ(n, m_game.readPosition().pieceAt(Square::D8));
        EXPECT_EQ(P, m_game.readPosition().pieceAt(Square::E7));
        EXPECT_EQ(1, material.blackKnights().count());
        EXPECT_EQ(1, material.whitePawns().count());
        EXPECT_EQ(0, material.whiteKnights().count());
        EXPECT_EQ(0, material.whiteQueens().count());
    }
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
//     A    B    C    D    E    F    G    H
// Each of the four castles made & unmade on its own, then
// 1. O-O-O Ra6
// 2. Rh3 O-O
//
// Result:
// 8 [   ][   ][   ][   ][   ][ r ][ k ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [ r ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][ R ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][ K ][ R ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// and unmade back to the start.
TEST_P(UnmakeFixture, Castling)
{
    auto k = BLACKKING;
    auto r = BLACKROOK;
    auto K = WHITEKING;
    auto R = WHITEROOK;
    auto empty = ChessPiece::None();

    struct CastleCase {
        const char* fen;
        Square kingFrom, kingTo, rookFrom, rookTo;
        ChessPiece king, rook;
        int castlingAfter;
    };
    const CastleCase cases[] = {
        { "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", Square::E1, Square::C1, Square::A1, Square::D1, K, R, CastlingState::BLACK_ALL },
        { "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", Square::E1, Square::G1, Square::H1, Square::F1, K, R, CastlingState::BLACK_ALL },
        { "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", Square::E8, Square::C8, Square::A8, Square::D8, k, r, CastlingState::WHITE_ALL },
        { "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", Square::E8, Square::G8, Square::H8, Square::F8, k, r, CastlingState::WHITE_ALL },
    };

    for (const auto& c : cases) {
        setup(c.fen);
        const Chessboard start = m_game.board();

        PackedMove castle = findMove(c.kingFrom, c.kingTo);
        EXPECT_TRUE(castle.isCastling());
        makeMove(castle);

        PositionReader position = m_game.readPosition();
        EXPECT_EQ(c.king, position.pieceAt(c.kingTo));
        EXPECT_EQ(c.rook, position.pieceAt(c.rookTo));
        EXPECT_EQ(empty, position.pieceAt(c.kingFrom));
        EXPECT_EQ(empty, position.pieceAt(c.rookFrom));
        EXPECT_EQ(static_cast<int>(c.castlingAfter), static_cast<int>(position.castling().read()));

        EXPECT_TRUE(m_game.unmakeMove());
        expectState(start, castle);
        EXPECT_TRUE(m_game.readPosition().castling().hasAll());
    }

    // 1. O-O-O Ra6 2. Rh3 O-O
    setup("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    std::vector<std::pair<Chessboard, PackedMove>> history;
    const std::pair<Square, Square> moves[] = {
        { Square::E1, Square::C1 }, { Square::A8, Square::A6 }, { Square::H1, Square::H3 }, { Square::E8, Square::G8 } };
    for (auto [source, target] : moves) {
        PackedMove move = findMove(source, target);
        history.emplace_back(makeMove(move), move);
    }

    {
        PositionReader position = m_game.readPosition();
        EXPECT_EQ(k, position.pieceAt(Square::G8));
        EXPECT_EQ(r, position.pieceAt(Square::F8));
        EXPECT_EQ(r, position.pieceAt(Square::A6));
        EXPECT_EQ(K, position.pieceAt(Square::C1));
        EXPECT_EQ(R, position.pieceAt(Square::D1));
        EXPECT_EQ(R, position.pieceAt(Square::H3));
        EXPECT_TRUE(position.castling().hasNone());
    }

    // undo
    for (auto itr = history.rbegin(); itr != history.rend(); ++itr) {
        EXPECT_TRUE(m_game.unmakeMove());
        expectState(itr->first, itr->second);
    }

    PositionReader position = m_game.readPosition();
    EXPECT_TRUE(position.castling().hasAll());
    EXPECT_EQ(k, position.pieceAt(Square::E8));
    EXPECT_EQ(K, position.pieceAt(Square::E1));
    EXPECT_EQ(r, position.pieceAt(Square::A8));
    EXPECT_EQ(r, position.pieceAt(Square::H8));
    EXPECT_EQ(R, position.pieceAt(Square::A1));
    EXPECT_EQ(R, position.pieceAt(Square::H1));
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][ b ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
//     A    B    C    D    E    F    G    H
// capturing a rook on its starting square takes away that side's castling, a rook capturing a rook takes away both.
TEST_P(UnmakeFixture, Castling_Captures)
{
    struct CaptureCase {
        const char* fen;
        Square source, target;
        ChessPiece capturer, captured;
        int castlingAfter;
    };
    const CaptureCase cases[] = {
        // Bxh1
        { "r3k2r/8/8/8/8/5b2/8/R3K2R b KQkq - 0 1", Square::F3, Square::H1, BLACKBISHOP, WHITEROOK,
            CastlingState::WHITE_QUEENSIDE | CastlingState::BLACK_ALL },
        // Rxa8
        { "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", Square::A1, Square::A8, WHITEROOK, BLACKROOK,
            CastlingState::WHITE_KINGSIDE | CastlingState::BLACK_KINGSIDE },
        // Rxa1
        { "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", Square::A8, Square::A1, BLACKROOK, WHITEROOK,
            CastlingState::WHITE_KINGSIDE | CastlingState::BLACK_KINGSIDE },
        // Rxh1
        { "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", Square::H8, Square::H1, BLACKROOK, WHITEROOK,
            CastlingState::WHITE_QUEENSIDE | CastlingState::BLACK_QUEENSIDE },
    };

    for (const auto& c : cases) {
        setup(c.fen);
        const Chessboard start = m_game.board();

        PackedMove capture = findMove(c.source, c.target);
        EXPECT_TRUE(capture.isCapture());
        makeMove(capture);

        PositionReader position = m_game.readPosition();
        EXPECT_NE(start.readPosition().hash(), position.hash());
        EXPECT_EQ(static_cast<int>(c.castlingAfter), static_cast<int>(position.castling().read())) << capture.toString();
        EXPECT_EQ(c.capturer, position.pieceAt(c.target));
        EXPECT_EQ(ChessPiece::None(), position.pieceAt(c.source));

        EXPECT_TRUE(m_game.unmakeMove());
        expectState(start, capture);
        EXPECT_TRUE(m_game.readPosition().castling().hasAll());
        EXPECT_EQ(c.capturer, m_game.readPosition().pieceAt(c.source));
        EXPECT_EQ(c.captured, m_game.readPosition().pieceAt(c.target));
    }
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [ R ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// Moves:
// 1... Ke7 2. Ke2, a king move takes away its side's castling, unmake gives it back.
TEST_P(UnmakeFixture, Unmake_KingMoves)
{
    auto k = BLACKKING;
    auto K = WHITEKING;
    setup("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1");
    const Chessboard start = m_game.board();
    EXPECT_EQ(static_cast<int>(CastlingState::WHITE_QUEENSIDE), static_cast<int>(m_game.readPosition().castling().read()));

    // Ke7
    PackedMove Ke7 = findMove(Square::E8, Square::E7);
    makeMove(Ke7);
    const Chessboard afterKe7 = m_game.board();
    EXPECT_NE(start.readPosition().hash(), m_game.readPosition().hash());
    EXPECT_EQ(k, m_game.readPosition().pieceAt(Square::E7));
    EXPECT_TRUE(m_game.readPosition().castling().hasWhiteQueenSide());

    // Ke2
    PackedMove Ke2 = findMove(Square::E1, Square::E2);
    makeMove(Ke2);
    EXPECT_NE(afterKe7.readPosition().hash(), m_game.readPosition().hash());
    EXPECT_EQ(K, m_game.readPosition().pieceAt(Square::E2));
    EXPECT_FALSE(m_game.readPosition().castling().hasAny());

    // undo
    EXPECT_TRUE(m_game.unmakeMove());
    expectState(afterKe7, Ke2);
    EXPECT_EQ(K, m_game.readPosition().pieceAt(Square::E1));
    EXPECT_TRUE(m_game.readPosition().castling().hasWhiteQueenSide());

    EXPECT_TRUE(m_game.unmakeMove());
    expectState(start, Ke7);
    EXPECT_EQ(k, m_game.readPosition().pieceAt(Square::E8));
    EXPECT_EQ(K, m_game.readPosition().pieceAt(Square::E1));

    // nothing left to unmake
    EXPECT_FALSE(m_game.unmakeMove());
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][ B ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_P(UnmakeFixture, BishopCaptureRookRemovesCastlingOption)
{
    setup("r3k3/8/8/3B4/8/8/8/4K3 w q - 0 1");
    const Chessboard start = m_game.board();
    const auto& material = m_game.readPosition().material();

    EXPECT_EQ(1, material.blackKing().count());
    EXPECT_EQ(1, material.blackRooks().count());
    EXPECT_EQ(1, material.whiteBishops().count());

    PackedMove Bxa8 = findMove(Square::D5, Square::A8);
    EXPECT_TRUE(Bxa8.isCapture());
    makeMove(Bxa8);

    EXPECT_TRUE(m_game.readPosition().castling().hasNone());
    EXPECT_EQ(WHITEBISHOP, m_game.readPosition().pieceAt(Square::A8));
    EXPECT_NE(start.readPosition().hash(), m_game.readPosition().hash());
    EXPECT_EQ(1, material.blackKing().count());
    EXPECT_EQ(0, material.blackRooks().count());
    EXPECT_EQ(1, material.whiteBishops().count());

    EXPECT_TRUE(m_game.unmakeMove());
    expectState(start, Bxa8);
    EXPECT_EQ(static_cast<int>(CastlingState::BLACK_QUEENSIDE), static_cast<int>(m_game.readPosition().castling().read()));
    EXPECT_EQ(WHITEBISHOP, m_game.readPosition().pieceAt(Square::D5));
    EXPECT_EQ(BLACKKING, m_game.readPosition().pieceAt(Square::E8));
    EXPECT_EQ(BLACKROOK, m_game.readPosition().pieceAt(Square::A8));
    EXPECT_EQ(1, material.blackKing().count());
    EXPECT_EQ(1, material.blackRooks().count());
    EXPECT_EQ(1, material.whiteBishops().count());
}

// every move two plies deep from kiwipete, quiet moves, captures, castles, en passant & promotions alike, unmakes back to
// the exact position it was made from.
TEST_P(UnmakeFixture, Unmake_EveryMoveTwoPliesDeep_RestoresState)
{
    setup("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    const Chessboard start = m_game.board();

    size_t replies = 0;
    for (PackedMove first : legalMoves()) {
        makeMove(first);
        const Chessboard afterFirst = m_game.board();
        for (PackedMove second : legalMoves()) {
            makeMove(second);
            EXPECT_TRUE(m_game.unmakeMove());
            expectState(afterFirst, second);
            ++replies;
        }
        EXPECT_TRUE(m_game.unmakeMove());
        expectState(start, first);
    }

    EXPECT_EQ(2039u, replies);
}

////////////////////////////////////////////////////////////////

ELEPHANT_MOVE_GEN_AB(UnmakeFixture);

}  // namespace ElephantTest
