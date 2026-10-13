#include <gtest/gtest.h>
#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <move/generation/move_gen_policy.hpp>
#include <move/generation/move_generator.hpp>
#include <move/move_executor.hpp>
#include <position/hash_zobrist.hpp>

#include <string>
#include <vector>

#include "chess_positions.hpp"

namespace ElephantTest {

namespace {
// Walks every legal line to the given depth and checks the incrementally updated hash against a full recompute after
// every make and unmake. Records the first line that diverges.
template<Set us>
void verifyHashThroughTree(GameContext& context, int depth, std::vector<std::string>& line, std::string& firstFailure) {
    if (depth == 0 || !firstFailure.empty())
        return;

    move_gen_policy::Tusk::forEachMove<us>(context.readChessPosition(), [&](PackedMove move) {
        if (!firstFailure.empty())
            return;

        const u64 before = context.readChessPosition().hash();
        line.push_back(move.toString());
        context.MakeMove(move);

        const u64 incremental = context.readChessPosition().hash();
        const u64 computed = zobrist::computeBoardHash(context.readChessboard());
        if (incremental != computed) {
            for (const auto& m : line) firstFailure += m + " ";
            firstFailure += "(after make)";
        }

        verifyHashThroughTree<opposing_set<us>()>(context, depth - 1, line, firstFailure);

        context.UnmakeMove();
        if (firstFailure.empty() && context.readChessPosition().hash() != before) {
            for (const auto& m : line) firstFailure += m + " ";
            firstFailure += "(after unmake)";
        }
        line.pop_back();
    });
}
} // namespace

TEST(ZobristHashing, IncrementalMatchesComputedThroughTree)
{
    zobrist::internals::initialize();

    const std::vector<std::pair<std::string, int>> positions = {
        { "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1", 4 },             // WAC.001
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3 },   // kiwipete
        { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4 },
        { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3 },
        { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3 },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1", 3 },   // kiwipete, black to move
    };

    for (const auto& [fen, depth] : positions) {
        GameContext context;
        io::fen_parser::deserialize(fen.c_str(), context.editChessboard());
        ASSERT_EQ(zobrist::computeBoardHash(context.readChessboard()), context.readChessPosition().hash()) << "initial hash, " << fen;

        std::vector<std::string> line;
        std::string firstFailure;
        if (context.readToPlay() == Set::WHITE)
            verifyHashThroughTree<Set::WHITE>(context, depth, line, firstFailure);
        else
            verifyHashThroughTree<Set::BLACK>(context, depth, line, firstFailure);

        EXPECT_TRUE(firstFailure.empty()) << fen << "\n  hash diverged after: " << firstFailure;
    }
}

TEST(ZobristHashing, Initialization)
{
    zobrist::internals::initialize();
    bool initialized = zobrist::internals::initialized();
    EXPECT_TRUE(initialized) << "Zobrist hashing should be initialized after calling initialize().";

    // calling initialize again should not change anything.
    zobrist::internals::initialize();
    initialized = zobrist::internals::initialized();
    EXPECT_TRUE(initialized) << "Zobrist hashing should remain initialized after calling initialize() again.";
}


TEST(ZobristHashing, Hashing)
{
    zobrist::internals::initialize();

    Chessboard board;
    u64 hash = zobrist::computeBoardHash(board);
    EXPECT_EQ(hash, 0) << "Empty board should have a hash of zero.";

    chess_positions::defaultStartingPosition(board.editPosition());
    hash = zobrist::computeBoardHash(board);
    EXPECT_NE(hash, 0) << "Default starting position should not have a hash of zero.";
    EXPECT_NE(board.readPosition().hash(), 0) << "Default starting position should not have a hash of zero."; 
    EXPECT_EQ(board.readPosition().hash(), hash) << "Hash from computeBoardHash should match position's stored hash.";
}

TEST(ZobristHashing, SamePositionSameHash)
{
    zobrist::internals::initialize();

    Chessboard boardOne;
    Chessboard boardTwo;

    u64 hashOne = zobrist::computeBoardHash(boardOne);
    u64 hashTwo = zobrist::computeBoardHash(boardTwo);

    EXPECT_EQ(hashOne, hashTwo) << "Two identical empty boards should have the same hash.";
}

TEST(ZobristHashing, DifferentPositionDifferentHash)
{
    zobrist::internals::initialize();

    Chessboard boardOne;
    Chessboard boardTwo;
    PositionEditor editorOne = boardOne.editPosition();
    PositionEditor editorTwo = boardTwo.editPosition();

    editorOne.placePieces(piece_constants::white_pawn, Square::E2);
    editorTwo.placePieces(piece_constants::black_pawn, Square::E2);

    u64 hashOne = zobrist::computeBoardHash(boardOne);
    u64 hashTwo = zobrist::computeBoardHash(boardTwo);

    EXPECT_NE(hashOne, hashTwo) << "Two different board positions should have different hashes.";
}

TEST(ZobristHashing, StartingPosition_EqualHash)
{
    zobrist::internals::initialize();

    Chessboard boardOne;
    Chessboard boardTwo;
    PositionEditor editorOne = boardOne.editPosition();
    PositionEditor editorTwo = boardTwo.editPosition();

    chess_positions::defaultStartingPosition(editorOne);
    chess_positions::defaultStartingPosition(editorTwo);

    u64 hashOne = zobrist::computeBoardHash(boardOne);
    u64 hashTwo = zobrist::computeBoardHash(boardTwo);

    EXPECT_EQ(hashOne, hashTwo) << "Two identical starting positions should have the same hash.";
    EXPECT_EQ(editorOne.hash(), editorTwo.hash()) << "Two identical starting positions should have the same hash.";
    EXPECT_EQ(hashOne, editorOne.hash()) << "Hash and editor hash should match for identical positions.";
}

TEST(ZobristHashing, PlacingPiecesAndHashingBoard_ShouldResultWithEqualHash)
{
    zobrist::internals::initialize();

    Chessboard board;
    PositionEditor editor = board.editPosition();

    chess_positions::defaultStartingPosition(editor);

    u64 initialHash = zobrist::computeBoardHash(board);

    editor.placePieces(piece_constants::white_pawn, Square::E4);
    u64 newHash = zobrist::computeBoardHash(board);

    EXPECT_NE(initialHash, newHash) << "Hash should change after placing a piece.";
    EXPECT_EQ(editor.hash(), newHash) << "Editor hash should match the new board hash.";

    editor.castling().revokeBlackKingSide();
    u64 boardHash = zobrist::computeBoardHash(board);
    EXPECT_NE(newHash, boardHash) << "Hash should change after revoking castling rights.";
    EXPECT_EQ(editor.hash(), boardHash) << "Editor hash should match the final board hash.";
}

TEST(ZobristHashing, MakeAndUnmakeMove_ShouldRestoreHash)
{
    zobrist::internals::initialize();

    GameContext game;
    game.NewGame();    

    PositionReader positionReader = game.readChessPosition();
    u64 initialHash = positionReader.hash();

    // 1. e4 d5 2. e5 f5, the en passant square f6 is only recorded because the e5 pawn could capture on it.
    game.MakeMove<true>(PackedMove(Square::E2, Square::E4));
    game.MakeMove<true>(PackedMove(Square::D7, Square::D5));
    game.MakeMove<true>(PackedMove(Square::E4, Square::E5));
    initialHash = positionReader.hash();
    PackedMove move(Square::F7, Square::F5);
    game.MakeMove<true>(move);
    
    u64 afterMoveHash = positionReader.hash();

    EXPECT_NE(initialHash, afterMoveHash) << "Hash should change after making a move.";
    EXPECT_TRUE(positionReader.enPassant()) << "En passant should be available after the move.";
    EXPECT_EQ(Square::F6, positionReader.enPassant().readSquare());

    // test clear and reset enPassant
    u64 hashWithEnPassant = positionReader.hash();
    Square epSqr = positionReader.enPassant().readSquare();
    PositionEditor editor = game.editChessPosition();
    editor.enPassant().clear();
    EXPECT_NE(hashWithEnPassant, positionReader.hash()) << "Hash should match after clearing en passant.";

    editor.enPassant().writeSquare(epSqr);
    EXPECT_EQ(hashWithEnPassant, positionReader.hash()) << "Hash should match after resetting en passant.";

    game.UnmakeMove();

    EXPECT_EQ(initialHash, positionReader.hash()) << "Hash should be restored to initial value after unmaking the move.";

}

TEST(ZobristHashing, HashToMove_Unmake_Rehash)
{
    zobrist::internals::initialize();

    GameContext game;
    game.NewGame();    

    PositionReader positionReader = game.readChessPosition();
    u64 initialHash = positionReader.hash();
    
    u64 newHash = zobrist::updateBlackToMoveHash(initialHash);
    EXPECT_NE(initialHash, newHash) << "Hash should change after updating black to move.";

    newHash = zobrist::updateBlackToMoveHash(newHash);
    EXPECT_EQ(initialHash, newHash) << "Hash should revert to initial value after updating";

}

// A FEN with black to move has to carry the black to move key, issue #13. Loading it gives the same hash as a full
// recompute, and the same hash as reaching the position by moves from a white to move FEN.
TEST(ZobristHashing, BlackToMoveFen_HashMatchesComputedAndPositionReachedByMoves)
{
    zobrist::internals::initialize();

    const std::string blackToMove = "r1bqkbnr/pppppppp/2n5/8/8/2N2N2/PPPPPPPP/R1BQKB1R b KQkq - 3 2";
    GameContext loaded;
    ASSERT_TRUE(io::fen_parser::deserialize(blackToMove, loaded.editChessboard()));
    ASSERT_EQ(Set::BLACK, loaded.readToPlay());
    EXPECT_EQ(zobrist::computeBoardHash(loaded.readChessboard()), loaded.readChessPosition().hash());

    // 1. Nf3 Nc6 2. Nc3, no pawn moves so neither an en passant square nor the halfmove clock tells the two apart.
    GameContext played;
    ASSERT_TRUE(io::fen_parser::deserialize(std::string(c_startPositionFen), played.editChessboard()));
    played.MakeMove(PackedMove(Square::G1, Square::F3));
    played.MakeMove(PackedMove(Square::B8, Square::C6));
    played.MakeMove(PackedMove(Square::B1, Square::C3));
    ASSERT_EQ(Set::BLACK, played.readToPlay());

    std::string playedFen;
    io::fen_parser::serialize(played.readChessboard(), playedFen);
    ASSERT_EQ(blackToMove, playedFen);

    EXPECT_EQ(played.readChessPosition().hash(), loaded.readChessPosition().hash());
    EXPECT_EQ(zobrist::computeBoardHash(played.readChessboard()), played.readChessPosition().hash());

    // the same position with white to move differs only by the black to move key.
    GameContext whiteToMove;
    ASSERT_TRUE(io::fen_parser::deserialize("r1bqkbnr/pppppppp/2n5/8/8/2N2N2/PPPPPPPP/R1BQKB1R w KQkq - 3 2", whiteToMove.editChessboard()));
    EXPECT_EQ(zobrist::updateBlackToMoveHash(whiteToMove.readChessPosition().hash()), loaded.readChessPosition().hash());

    // reloading a black to move FEN into a board that already has black to move keeps the key exactly once.
    ASSERT_TRUE(io::fen_parser::deserialize(blackToMove, loaded.editChessboard()));
    EXPECT_EQ(played.readChessPosition().hash(), loaded.readChessPosition().hash());
}


////////////////////////////////////////////////////////////////
// En passant squares are only recorded (and hashed) when an enemy pawn stands beside the double pushed pawn, otherwise
// the position is the same as one reached without a double push and repetition detection needs equal hashes.

static u64 hashAfterMoves(const char* fen, std::initializer_list<PackedMove> moves, bool* hasEnPassant = nullptr)
{
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    for (const auto& move : moves)
        context.MakeMove<true>(move);
    if (hasEnPassant)
        *hasEnPassant = context.readChessPosition().enPassant();
    return context.readChessPosition().hash();
}

TEST(ZobristHashing, DoublePush_NoAdjacentEnemyPawn_NoEnPassantAndSameHashAsQuietMove)
{
    zobrist::internals::initialize();

    struct Case { const char* fenDoublePush; const char* fenSinglePush; Square from; Square to; };
    const Case cases[] = {
        // lone pawn
        { "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1", "4k3/8/8/8/8/4P3/8/4K3 w - - 0 1", Square::E2, Square::E4 },
        // black pawn two files away
        { "4k3/8/8/8/2p5/8/4P3/4K3 w - - 0 1", "4k3/8/8/8/2p5/4P3/8/4K3 w - - 0 1", Square::E2, Square::E4 },
        // black pawn on an adjacent file but the wrong rank
        { "4k3/8/8/3p4/8/8/4P3/4K3 w - - 0 1", "4k3/8/8/3p4/8/4P3/8/4K3 w - - 0 1", Square::E2, Square::E4 },
        // h-file push, a black pawn on a5 mustn't wrap around as a neighbour
        { "4k3/8/8/p7/8/8/7P/4K3 w - - 0 1", "4k3/8/8/p7/8/7P/8/4K3 w - - 0 1", Square::H2, Square::H4 },
        // black a-file push, a white pawn on h5 mustn't wrap around either
        { "4k3/p7/8/7P/8/8/8/4K3 b - - 0 1", "4k3/8/p7/7P/8/8/8/4K3 b - - 0 1", Square::A7, Square::A5 },
        // white pawn on an adjacent file but a pawn of the pushing side
        { "4k3/p7/8/1p6/8/8/8/4K3 b - - 0 1", "4k3/8/p7/1p6/8/8/8/4K3 b - - 0 1", Square::A7, Square::A5 },
    };

    for (const auto& c : cases) {
        bool hasEnPassant = true;
        const u64 doublePush = hashAfterMoves(c.fenDoublePush, { PackedMove(c.from, c.to) }, &hasEnPassant);
        EXPECT_FALSE(hasEnPassant) << c.fenDoublePush;

        // the same position, reached with a single step from one rank further.
        const Square middle = static_cast<Square>((to_index(c.from) + to_index(c.to)) / 2);
        const u64 singlePush = hashAfterMoves(c.fenSinglePush, { PackedMove(middle, c.to) });
        EXPECT_EQ(doublePush, singlePush) << c.fenDoublePush;
    }
}

TEST(ZobristHashing, DoublePush_AdjacentEnemyPawn_EnPassantSetAndCapturable)
{
    zobrist::internals::initialize();

    for (const Square capturer : { Square::D4, Square::F4 }) {
        GameContext context;
        io::fen_parser::deserialize("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1", context.editChessboard());
        context.editChessPosition().placePiece(BLACKPAWN, capturer);
        const u64 beforePush = context.readChessPosition().hash();

        context.MakeMove<true>(PackedMove(Square::E2, Square::E4));
        PositionReader position = context.readChessPosition();
        ASSERT_TRUE(position.enPassant());
        EXPECT_EQ(Square::E3, position.enPassant().readSquare());
        EXPECT_EQ(zobrist::computeBoardHash(context.readChessboard()), position.hash());
        const u64 afterPush = position.hash();

        // the generator offers the en passant capture.
        MoveGenParams params;
        params.setPawns(true);
        MoveGenerator<Set::BLACK> gen(position, params);
        PackedMove epCapture;
        while (auto move = gen.pop().move) {
            if (move.isEnPassant())
                epCapture = move;
        }
        ASSERT_FALSE(epCapture.isNull());
        EXPECT_EQ(capturer, epCapture.sourceSqr());
        EXPECT_EQ(Square::E3, epCapture.targetSqr());

        context.MakeMove<true>(epCapture);
        // position reads the live board.
        EXPECT_EQ(ChessPiece::None(), position.pieceAt(Square::E4));
        EXPECT_EQ(BLACKPAWN, position.pieceAt(Square::E3));
        EXPECT_FALSE(position.enPassant());
        EXPECT_EQ(zobrist::computeBoardHash(context.readChessboard()), position.hash());

        EXPECT_TRUE(context.UnmakeMove());
        EXPECT_EQ(afterPush, context.readChessPosition().hash());
        EXPECT_EQ(Square::E3, context.readChessPosition().enPassant().readSquare());
        EXPECT_TRUE(context.UnmakeMove());
        EXPECT_EQ(beforePush, context.readChessPosition().hash());
    }
}

TEST(ZobristHashing, Fen_EnPassantWithoutCapturer_DroppedAndHashesAsNone)
{
    zobrist::internals::initialize();

    struct Case { const char* withSquare; const char* withoutSquare; bool kept; };
    const Case cases[] = {
        // 1. e4 from the start position, no black pawn on d4/f4.
        { "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1", false },
        // 1. e4 c5, the c5 pawn isn't beside a white pawn.
        { "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2", "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2", false },
        // the white pawn on e5 can capture d6 en passant, the square stays.
        { "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3", "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3", true },
    };

    for (const auto& c : cases) {
        GameContext withSquare;
        GameContext withoutSquare;
        ASSERT_TRUE(io::fen_parser::deserialize(c.withSquare, withSquare.editChessboard()));
        ASSERT_TRUE(io::fen_parser::deserialize(c.withoutSquare, withoutSquare.editChessboard()));

        EXPECT_EQ(c.kept, static_cast<bool>(withSquare.readChessPosition().enPassant())) << c.withSquare;
        if (c.kept)
            EXPECT_NE(withoutSquare.readChessPosition().hash(), withSquare.readChessPosition().hash()) << c.withSquare;
        else
            EXPECT_EQ(withoutSquare.readChessPosition().hash(), withSquare.readChessPosition().hash()) << c.withSquare;

        // serializing writes the square only when it was kept.
        std::string fen;
        io::fen_parser::serialize(withSquare.readChessboard(), fen);
        EXPECT_EQ(std::string(c.kept ? c.withSquare : c.withoutSquare), fen);
    }
}

} // namespace ElephantTest
