#include <gtest/gtest.h>
#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <move/generation/move_gen_policy.hpp>
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

    PackedMove move(Square::E2, Square::E4);
    game.MakeMove<true>(move);
    
    u64 afterMoveHash = positionReader.hash();

    EXPECT_NE(initialHash, afterMoveHash) << "Hash should change after making a move.";
    EXPECT_TRUE(positionReader.enPassant()) << "En passant should be available after the move.";

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

} // namespace ElephantTest