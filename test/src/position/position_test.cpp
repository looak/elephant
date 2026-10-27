#include <gtest/gtest.h>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <bitboard/attacks/attacks.hpp>
#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <material/chess_piece.hpp>
#include <material/material_mask.hpp>
#include <material/material_topology.hpp>
#include <move/generation/move_gen_policy.hpp>
#include <position/castling_state_info.hpp>
#include <position/en_passant_state_info.hpp>
#include <position/hash_zobrist.hpp>
#include <position/position.hpp>
#include <position/position_accessors.hpp>
#include <position/position_hash.hpp>

#include "bitboard_test_helpers.hpp"
#include "chess_positions.hpp"

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file position_test.cpp
 * @brief The position and what it's built from: the material bitboards, castling & en passant state, the hash kept up to
 * date by every edit, read & edit access through the proxies, iterating the board, the squares the material threatens
 * and FEN round trips of positions built piece by piece. Move generation, make/unmake and perft have suites of their own.
 * Every test that sets up a position draws it above the test, white in uppercase & black in lowercase, x marks the
 * squares the test is about.
 * Naming convention: <Area>_<Situation>_<Expectation>
 * @author Alexander Loodin Ek */

namespace {
using enum Square;

// the pieces by their FEN letter, as the boards above the tests draw them.
constexpr ChessPiece K = piece_constants::white_king;
constexpr ChessPiece Q = piece_constants::white_queen;
constexpr ChessPiece R = piece_constants::white_rook;
constexpr ChessPiece B = piece_constants::white_bishop;
constexpr ChessPiece N = piece_constants::white_knight;
constexpr ChessPiece P = piece_constants::white_pawn;
constexpr ChessPiece k = piece_constants::black_king;
constexpr ChessPiece q = piece_constants::black_queen;
constexpr ChessPiece r = piece_constants::black_rook;
constexpr ChessPiece b = piece_constants::black_bishop;
constexpr ChessPiece n = piece_constants::black_knight;
constexpr ChessPiece p = piece_constants::black_pawn;

// the hash of the position computed from scratch, white to move.
u64 computedHash(PositionReader position) { return zobrist::computePositionHash(position, true); }

Bitboard squares(std::initializer_list<Square> list)
{
    Bitboard result;
    for (Square sqr : list)
        result[sqr] = true;
    return result;
}
} // namespace

////////////////////////////////////////////////////////////////
// Position: the basics, empty, placing & removing, the starting position, copies and equality.
class PositionFixture : public ::testing::Test {
public:
    Position m_position;
};

TEST_F(PositionFixture, Position_Default_EmptyWithNoRightsNoEnPassantAndZeroHash)
{
    const PositionReader reader = m_position.read();
    EXPECT_TRUE(reader.empty());
    EXPECT_TRUE(reader.material().combine().empty());
    EXPECT_TRUE(reader.castling().hasNone());
    EXPECT_FALSE(reader.enPassant());
    EXPECT_EQ(0u, reader.hash());
    for (u8 index = 0; index < 64; ++index)
        EXPECT_FALSE(reader.pieceAt(static_cast<Square>(index)).isValid()) << to_string(static_cast<Square>(index));
}

// 8 [ . ][ . ][ . ][ n ][ b ][ r ][ q ][ k ]
// 7 [ . ][ . ][ p ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ P ][ . ][ . ]
// 1 [ K ][ Q ][ R ][ B ][ N ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Position_OneOfEachPiece_PieceAtReadsEachBackAndTheRestIsEmpty)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, A1, Q, B1, R, C1, B, D1, N, E1, P, F2, k, H8, q, G8, r, F8, b, E8, n, D8, p, C7);

    // validate
    const std::vector<std::pair<Square, ChessPiece>> expected = {
        { A1, K }, { B1, Q }, { C1, R }, { D1, B }, { E1, N }, { F2, P },
        { H8, k }, { G8, q }, { F8, r }, { E8, b }, { D8, n }, { C7, p } };
    for (const auto& [sqr, piece] : expected)
        EXPECT_EQ(piece, editor.pieceAt(sqr)) << to_string(sqr);

    EXPECT_EQ(12, editor.material().combine().count());
    EXPECT_EQ(6, editor.material().white().count());
    EXPECT_EQ(6, editor.material().black().count());
    EXPECT_FALSE(editor.pieceAt(A2).isValid());
    EXPECT_FALSE(editor.pieceAt(H1).isValid());
}

// 8 [ r ][ n ][ b ][ q ][ k ][ b ][ n ][ r ]
// 7 [ p ][ p ][ p ][ p ][ p ][ p ][ p ][ p ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ P ][ P ][ P ][ P ][ P ][ P ][ P ][ P ]
// 1 [ R ][ N ][ B ][ Q ][ K ][ B ][ N ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Position_StartingPosition_SixteenPiecesASideAndAllCastlingRights)
{
    // setup
    chess_positions::defaultStartingPosition(m_position.edit());
    const auto material = m_position.read().material();

    // validate
    EXPECT_EQ(16, material.white().count());
    EXPECT_EQ(16, material.black().count());
    EXPECT_EQ(8, material.whitePawns().count());
    EXPECT_EQ(8, material.blackPawns().count());
    EXPECT_EQ(2, material.whiteKnights().count());
    EXPECT_EQ(2, material.blackKnights().count());
    EXPECT_EQ(2, material.whiteBishops().count());
    EXPECT_EQ(2, material.blackBishops().count());
    EXPECT_EQ(2, material.whiteRooks().count());
    EXPECT_EQ(2, material.blackRooks().count());
    EXPECT_EQ(1, material.whiteQueens().count());
    EXPECT_EQ(1, material.blackQueens().count());
    EXPECT_EQ(1, material.whiteKing().count());
    EXPECT_EQ(1, material.blackKing().count());
    EXPECT_TRUE(material.whiteKing()[E1]);
    EXPECT_TRUE(material.blackKing()[E8]);
    EXPECT_TRUE(material.whiteQueens()[D1]);
    EXPECT_TRUE(material.blackQueens()[D8]);

    // ranks 3 to 6 are empty.
    EXPECT_EQ(0, (material.combine() & Bitboard(0x0000ffffffff0000ull)).count());
    EXPECT_TRUE(m_position.read().castling().hasAll());
    EXPECT_FALSE(m_position.read().enPassant());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ n ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ B ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ R ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Position_PlaceThenClearEveryPiece_EqualsAnEmptyPosition)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(R, A1, K, E1, B, C3, n, D5, k, E8);

    // do
    for (Square sqr : { C3, A1, E8, D5, E1 })
        editor.clearPiece(sqr);

    // validate
    EXPECT_TRUE(editor.empty());
    EXPECT_EQ(0u, editor.hash());
    EXPECT_EQ(Position(), m_position);
}

// 8 [ r ][ n ][ b ][ q ][ k ][ b ][ n ][ r ]
// 7 [ p ][ p ][ p ][ . ][ p ][ p ][ p ][ p ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ p ][ P ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ P ][ P ][ P ][ P ][ . ][ P ][ P ][ P ]
// 1 [ R ][ N ][ B ][ Q ][ K ][ B ][ N ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Position_Clear_RemovesPiecesRightsEnPassantAndHash)
{
    // setup, black's d4 pawn could take e4 en passant on e3.
    auto editor = m_position.edit();
    ASSERT_TRUE(io::fen_parser::deserialize("rnbqkbnr/ppp1pppp/8/8/3pP3/8/PPPP1PPP/RNBQKBNR", editor));
    editor.castling().grantAll();
    editor.enPassant().writeSquare(E3);
    ASSERT_NE(0u, editor.hash());

    // do
    editor.clear();

    // validate
    EXPECT_TRUE(editor.empty());
    EXPECT_TRUE(editor.castling().hasNone());
    EXPECT_FALSE(editor.enPassant());
    EXPECT_EQ(0u, editor.hash());
    EXPECT_EQ(Position(), m_position);
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Position_Copy_EditingTheCopyLeavesTheOriginal)
{
    // setup
    m_position.edit().placePieces(K, E1, k, E8);

    // do
    Position copy = m_position.read().copy();
    ASSERT_EQ(m_position, copy);
    copy.edit().placePiece(Q, D1);

    // validate
    EXPECT_FALSE(m_position.read().pieceAt(D1).isValid());
    EXPECT_EQ(Q, copy.read().pieceAt(D1));
    EXPECT_FALSE(m_position == copy);
    EXPECT_NE(m_position.read().hash(), copy.read().hash());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Position_Assignment_TakesPiecesStateAndHash)
{
    // setup
    m_position.edit().placePieces(K, E1, k, E8);
    m_position.edit().castling().grantWhiteKingSide();
    Position other; // a white queen on d4 alone, overwritten below.
    other.edit().placePieces(Q, D4);

    // do
    other = m_position;

    // validate
    EXPECT_EQ(m_position, other);
    EXPECT_FALSE(other.read().pieceAt(D4).isValid());
    EXPECT_TRUE(other.read().castling().hasWhiteKingSide());
    EXPECT_EQ(m_position.read().hash(), other.read().hash());
}

// 8 [ r ][ . ][ . ][ . ][ k ][ . ][ . ][ r ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ R ][ . ][ . ][ . ][ K ][ . ][ . ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Position_SamePiecesDifferentCastlingRights_NotEqual)
{
    // setup
    Position other;
    m_position.edit().placePieces(R, A1, K, E1, R, H1, r, A8, k, E8, r, H8);
    other.edit().placePieces(R, A1, K, E1, R, H1, r, A8, k, E8, r, H8);
    ASSERT_EQ(m_position, other);

    // do
    m_position.edit().castling().grantAll();
    other.edit().castling().grantAllWhite();

    // validate
    EXPECT_FALSE(m_position == other);
    EXPECT_EQ(m_position.read().material(), other.read().material());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ p ][ P ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Position_SamePiecesOneWithEnPassant_NotEqual)
{
    // setup
    Position other;
    m_position.edit().placePieces(K, E1, P, E4, p, D4, k, E8);
    other.edit().placePieces(K, E1, P, E4, p, D4, k, E8);

    // do
    m_position.edit().enPassant().writeSquare(E3);

    // validate
    EXPECT_FALSE(m_position == other);
    EXPECT_NE(m_position.read().hash(), other.read().hash());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ B ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Position_SameSquareDifferentPieceType_NotEqual)
{
    // setup
    Position other;
    m_position.edit().placePieces(K, E1, B, C3, k, E8);
    other.edit().placePieces(K, E1, N, C3, k, E8);

    // validate, the other position has a white knight on c3 instead. Both have a white piece there, the set boards are
    // the same and only the piece boards tell them apart.
    EXPECT_EQ(m_position.read().material().white(), other.read().material().white());
    EXPECT_FALSE(m_position == other);
}

// 8 [ r ][ . ][ . ][ . ][ k ][ . ][ . ][ r ]
// 7 [ p ][ . ][ p ][ p ][ q ][ p ][ b ][ . ]
// 6 [ b ][ n ][ . ][ . ][ p ][ n ][ p ][ . ]
// 5 [ . ][ . ][ . ][ P ][ N ][ . ][ . ][ . ]
// 4 [ . ][ p ][ . ][ . ][ P ][ . ][ . ][ . ]
// 3 [ . ][ . ][ N ][ . ][ . ][ Q ][ . ][ p ]
// 2 [ P ][ P ][ P ][ B ][ B ][ P ][ P ][ P ]
// 1 [ R ][ . ][ . ][ . ][ K ][ . ][ . ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Position_SamePiecesPlacedInOppositeOrder_EqualWithTheSameHash)
{
    // setup
    const std::vector<std::pair<ChessPiece, Square>> pieces = {
        { r, A8 }, { k, E8 }, { r, H8 }, { p, A7 }, { p, C7 }, { p, D7 }, { q, E7 }, { p, F7 }, { b, G7 },
        { b, A6 }, { n, B6 }, { p, E6 }, { n, F6 }, { p, G6 }, { P, D5 }, { N, E5 }, { p, B4 }, { P, E4 },
        { N, C3 }, { Q, F3 }, { p, H3 }, { P, A2 }, { P, B2 }, { P, C2 }, { B, D2 }, { B, E2 }, { P, F2 },
        { P, G2 }, { P, H2 }, { R, A1 }, { K, E1 }, { R, H1 } };
    Position reversed;

    // do
    for (const auto& [piece, sqr] : pieces)
        m_position.edit().placePiece(piece, sqr);
    for (auto itr = pieces.rbegin(); itr != pieces.rend(); ++itr)
        reversed.edit().placePiece(itr->first, itr->second);

    // validate
    EXPECT_EQ(m_position, reversed);
    EXPECT_EQ(m_position.read().hash(), reversed.read().hash());
    EXPECT_EQ(32, m_position.read().material().combine().count());
}

////////////////////////////////////////////////////////////////
// Edit access: placing & clearing with and without validation, the square operator, the material editor, restoring
// state from an undo, and reading through an editor.
class PositionEditFixture : public ::testing::Test {
public:
    Position m_position;
};

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ P ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, PlacePiece_ValidatedOnAnOccupiedSquare_RefusedAndNothingChanges)
{
    // setup
    auto editor = m_position.edit();
    editor.placePiece(P, E4);
    const Position before = m_position;

    // do
    const bool placed = editor.placePiece<true>(n, E4);

    // validate
    EXPECT_FALSE(placed);
    EXPECT_EQ(P, editor.pieceAt(E4));
    EXPECT_TRUE(editor.material().knights().empty());
    EXPECT_TRUE(editor.material().black().empty());
    EXPECT_EQ(before, m_position);
}

TEST_F(PositionEditFixture, PlacePiece_ValidatedNoPiece_RefusedAndTheBoardStaysEmpty)
{
    auto editor = m_position.edit();
    EXPECT_FALSE(editor.placePiece<true>(ChessPiece(), D4));
    EXPECT_TRUE(editor.empty());
    EXPECT_EQ(0u, editor.hash());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, PlacePiece_ValidatedOnAnEmptySquare_PlacedAndHashed)
{
    // do
    auto editor = m_position.edit();
    const bool placed = editor.placePiece<true>(q, D4);

    // validate
    EXPECT_TRUE(placed);
    EXPECT_EQ(q, editor.pieceAt(D4));
    EXPECT_TRUE(editor.material().blackQueens()[D4]);
    EXPECT_EQ(computedHash(editor), editor.hash());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, ClearPiece_ValidatedOnAnEmptySquare_RefusedAndHashUnchanged)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, E1, k, E8);
    const u64 hash = editor.hash();

    // validate
    EXPECT_FALSE(editor.clearPiece<true>(D4));
    EXPECT_EQ(hash, editor.hash());
    EXPECT_EQ(2, editor.material().combine().count());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, ClearPiece_ValidatedKing_RefusedTheKingStays)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, E1, k, E8);

    // validate, neither king can be taken off a validated board.
    EXPECT_FALSE(editor.clearPiece<true>(E1));
    EXPECT_FALSE(editor.clearPiece<true>(E8));
    EXPECT_EQ(K, editor.pieceAt(E1));
    EXPECT_EQ(k, editor.pieceAt(E8));
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, ClearPiece_UnvalidatedKing_Removed)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, E1, k, E8);

    // do, setting up a position piece by piece goes through boards without kings.
    EXPECT_TRUE(editor.clearPiece(E1));

    // validate
    EXPECT_FALSE(editor.pieceAt(E1).isValid());
    EXPECT_TRUE(editor.material().whiteKing().empty());
    EXPECT_EQ(computedHash(editor), editor.hash());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ r ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, ClearPiece_ValidatedRook_RemovedFromItsSetAndPieceBoards)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, E1, r, D5, k, E8);

    // do
    EXPECT_TRUE(editor.clearPiece<true>(D5));

    // validate
    EXPECT_TRUE(editor.material().rooks().empty());
    EXPECT_FALSE(editor.material().black()[D5]);
    EXPECT_EQ(1, editor.material().black().count());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ N ][ . ][ N ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, ClearPieceKnown_TwoKnights_OnlyTheNamedSquareIsCleared)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, E1, N, C4, N, E4, k, E8);

    // do, the unmake overload is told which piece stands there.
    editor.clearPiece(N, C4);

    // validate
    EXPECT_EQ(squares({ E4 }), editor.material().whiteKnights());
    EXPECT_FALSE(editor.material().white()[C4]);
    EXPECT_EQ(computedHash(editor), editor.hash());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, PlacePieces_ManyPairs_EveryPairPlaced)
{
    // setup
    auto editor = m_position.edit();

    // do
    const bool placed = editor.placePieces(K, E1, k, E8);

    // validate
    EXPECT_TRUE(placed);
    EXPECT_EQ(K, editor.pieceAt(E1));
    EXPECT_EQ(k, editor.pieceAt(E8));
    EXPECT_EQ(2, editor.material().combine().count());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ x ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, SquareOperator_AssignToAnEmptySquare_PlacesThePiece)
{
    // do
    auto editor = m_position.edit();
    editor[G5] = b;

    // validate
    EXPECT_EQ(b, editor.pieceAt(G5));
    const PositionReader reader = editor.asReader();
    EXPECT_EQ(b, reader[G5]);
    EXPECT_TRUE(editor.material().blackBishops()[G5]);
    EXPECT_EQ(computedHash(editor), editor.hash());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ n ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, SquareOperator_AssignOverAnEnemyPiece_ReplacesIt)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, E1, n, D4, k, E8);

    // do
    editor[D4] = Q;

    // validate, the knight is gone from the black set and the knight board, the queen is on both white boards.
    EXPECT_EQ(Q, editor.pieceAt(D4));
    EXPECT_TRUE(editor.material().knights().empty());
    EXPECT_FALSE(editor.material().black()[D4]);
    EXPECT_TRUE(editor.material().whiteQueens()[D4]);
    EXPECT_EQ(1, editor.material().black().count());
    EXPECT_EQ(computedHash(editor), editor.hash());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ n ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, SquareOperator_AssignNoPiece_ClearsTheSquare)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, E1, n, D4, k, E8);

    // do
    editor[D4] = ChessPiece();

    // validate
    EXPECT_FALSE(editor.pieceAt(D4).isValid());
    EXPECT_TRUE(editor.material().knights().empty());
    EXPECT_EQ(computedHash(editor), editor.hash());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, MaterialEditor_SetThenUnsetASquare_WritesBothTheSetAndPieceBoards)
{
    // setup
    auto editor = m_position.edit();
    MutableMaterialProxy rooks = editor.materialEditor(Set::WHITE, PieceType::ROOK);

    // do
    rooks[A1] = true;

    // validate
    EXPECT_EQ(R, editor.pieceAt(A1));
    EXPECT_TRUE(editor.material().white()[A1]);
    EXPECT_TRUE(editor.material().rooks()[A1]);

    // do, unset
    rooks[A1] = false;

    // validate
    EXPECT_TRUE(editor.empty());
    EXPECT_TRUE(editor.material().rooks().empty());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ x ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, MaterialEditor_ConstRead_TrueOnlyWhereThatPieceStands)
{
    // setup
    auto editor = m_position.edit();
    editor.materialEditor(Set::BLACK, PieceType::QUEEN)[H8] = true;
    const MutableMaterialProxy queens = editor.materialEditor(Set::BLACK, PieceType::QUEEN);

    // validate
    EXPECT_TRUE(queens[H8]);
    EXPECT_FALSE(queens[G8]);
    EXPECT_FALSE(queens[A1]);
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, RestoreState_WritesEnPassantCastlingAndHashAsGiven)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, E1, k, E8);
    EnPassantStateInfo enPassant{};
    enPassant.writeSquare(D6);
    const u64 storedHash = 0x0123456789abcdefull;

    // do, the hash is taken as stored, not recomputed.
    editor.restoreState(enPassant.read(), static_cast<byte>(CastlingState::WHITE_QUEENSIDE | CastlingState::BLACK_KINGSIDE), storedHash);

    // validate
    EXPECT_EQ(D6, editor.enPassant().readSquare());
    EXPECT_TRUE(editor.castling().hasWhiteQueenSide());
    EXPECT_TRUE(editor.castling().hasBlackKingSide());
    EXPECT_FALSE(editor.castling().hasWhiteKingSide());
    EXPECT_FALSE(editor.castling().hasBlackQueenSide());
    EXPECT_EQ(storedHash, editor.hash());
    EXPECT_EQ(K, editor.pieceAt(E1));
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, AsReader_SeesEditsMadeAfterItWasTaken)
{
    // setup
    auto editor = m_position.edit();
    const PositionReader reader = editor.asReader();
    PositionReader converted = editor;
    ASSERT_TRUE(reader.empty());

    // do
    editor.placePiece(n, C6);
    editor.castling().grantBlackQueenSide();

    // validate, a reader is a view of the position, not a copy.
    EXPECT_EQ(n, reader.pieceAt(C6));
    EXPECT_EQ(n, converted.pieceAt(C6));
    EXPECT_TRUE(reader.castling().hasBlackQueenSide());
    EXPECT_EQ(editor.hash(), reader.hash());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionEditFixture, Copy_FromAnEditorAndAReader_BothEqualTheOriginal)
{
    // setup
    m_position.edit().placePieces(K, E1, k, E8);
    m_position.edit().castling().grantAllBlack();

    // validate
    EXPECT_EQ(m_position, m_position.edit().copy());
    EXPECT_EQ(m_position, m_position.read().copy());
}

} // namespace ElephantTest
