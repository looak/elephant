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

////////////////////////////////////////////////////////////////
// Board iteration: the iterator walks a1 to h8, reads what pieceAt reads and writes through the position's bitboards.
class PositionIteratorFixture : public ::testing::Test {
public:
    Position m_position;
};

// 8 [ r ][ . ][ . ][ . ][ k ][ . ][ . ][ r ]
// 7 [ p ][ . ][ p ][ p ][ q ][ p ][ b ][ . ]
// 6 [ b ][ n ][ . ][ . ][ p ][ n ][ p ][ . ]
// 5 [ . ][ . ][ . ][ P ][ N ][ . ][ . ][ . ]
// 4 [ . ][ p ][ . ][ . ][ P ][ . ][ . ][ . ]
// 3 [ . ][ . ][ N ][ . ][ . ][ Q ][ . ][ p ]
// 2 [ P ][ P ][ P ][ B ][ B ][ P ][ P ][ P ]
// 1 [ R ][ . ][ . ][ . ][ K ][ . ][ . ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionIteratorFixture, Iterator_Kiwipete_GetMatchesPieceAtOnEverySquare)
{
    // setup
    ASSERT_TRUE(io::fen_parser::deserialize("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", m_position.edit()));
    const PositionReader reader = m_position.read();

    // do
    int visited = 0;
    int pieces = 0;
    for (auto itr = reader.begin(); itr != reader.end(); ++itr) {
        EXPECT_EQ(reader.pieceAt(itr.square()), itr.get()) << to_string(itr.square());
        pieces += itr.get().isValid() ? 1 : 0;
        ++visited;
    }

    // validate
    EXPECT_EQ(64, visited);
    EXPECT_EQ(reader.material().combine().count(), pieces);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionIteratorFixture, Iterator_EmptyBoard_EveryGetIsNoPiece)
{
    const PositionReader reader = m_position.read();
    for (auto itr = reader.begin(); !itr.end(); ++itr)
        EXPECT_FALSE(itr.get().isValid()) << to_string(itr.square());
}

TEST_F(PositionIteratorFixture, Iterator_PostIncrement_ReturnsTheSquareBeforeTheStep)
{
    const PositionReader reader = m_position.read();
    auto itr = reader.begin();

    auto before = itr++;
    EXPECT_EQ(A1, before.square());
    EXPECT_EQ(B1, itr.square());

    auto& same = ++itr;
    EXPECT_EQ(C1, same.square());
    EXPECT_EQ(&itr, &same);
}

TEST_F(PositionIteratorFixture, Iterator_SixtyFourSteps_ReachesEndAndEqualsIt)
{
    const PositionReader reader = m_position.read();
    auto itr = reader.begin();
    itr += 63;
    EXPECT_EQ(H8, itr.square());
    EXPECT_FALSE(itr.end());
    EXPECT_NE(reader.end(), itr);

    ++itr;
    EXPECT_TRUE(itr.end());
    EXPECT_EQ(reader.end(), itr);
}

TEST_F(PositionIteratorFixture, Iterator_CopyAssignment_TakesTheOtherIteratorsSquare)
{
    const PositionReader reader = m_position.read();
    auto first = reader.begin();
    auto second = reader.begin();
    second += 20;

    first = second;

    EXPECT_EQ(second, first);
    EXPECT_EQ(E3, first.square());
    EXPECT_EQ(4, first.file());
    EXPECT_EQ(2, first.rank());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionIteratorFixture, Iterator_SetOnAnEmptySquare_PlacesThePieceAndKeepsTheHash)
{
    // setup
    auto editor = m_position.edit();
    auto itr = editor.begin();
    ++itr;

    // do
    itr.set(N);

    // validate
    EXPECT_EQ(N, editor.pieceAt(B1));
    EXPECT_TRUE(editor.material().whiteKnights()[B1]);
    EXPECT_EQ(computedHash(editor), editor.hash());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ R ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionIteratorFixture, Iterator_SetOverAnotherPiece_TheOldPieceIsGoneFromBothBoards)
{
    // setup
    auto editor = m_position.edit();
    editor.placePiece(R, A1);

    // do
    editor.begin().set(n);

    // validate
    EXPECT_EQ(n, editor.pieceAt(A1));
    EXPECT_TRUE(editor.material().rooks().empty());
    EXPECT_TRUE(editor.material().white().empty());
    EXPECT_TRUE(editor.material().blackKnights()[A1]);
    EXPECT_EQ(computedHash(editor), editor.hash());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ R ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionIteratorFixture, Iterator_SetNoPiece_ClearsTheSquare)
{
    // setup
    auto editor = m_position.edit();
    editor.placePiece(R, A1);

    // do
    editor.begin().set(ChessPiece());

    // validate
    EXPECT_TRUE(editor.empty());
    EXPECT_TRUE(editor.material().rooks().empty());
    EXPECT_EQ(0u, editor.hash());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionIteratorFixture, Iterator_SetEverySquareFromAString_BuildsTheSamePositionAsPlacing)
{
    // setup, the board of the starting position written square by square through the iterator.
    const std::string_view board = "RNBQKBNRPPPPPPPP................................pppppppprnbqkbnr";
    auto editor = m_position.edit();

    // do
    for (auto itr = editor.begin(); !itr.end(); ++itr) {
        ChessPiece piece;
        if (piece.fromString(board[itr.index()]))
            itr.set(piece);
    }

    // validate
    Position expected;
    chess_positions::defaultStartingPosition(expected.edit());
    EXPECT_EQ(expected.read().material(), m_position.read().material());
    EXPECT_EQ(computedHash(editor), editor.hash());
}

namespace {
constexpr ChessPiece c_allPieces[12] = { K, Q, R, B, N, P, k, q, r, b, n, p };
} // namespace

////////////////////////////////////////////////////////////////
// Hashing: whatever edits the position, the hash it keeps equals the hash computed from scratch.
class PositionHashFixture : public ::testing::Test {
public:
    Position m_position;
    GameContext m_game;

    void setup(std::string_view fen) { ASSERT_TRUE(io::fen_parser::deserialize(fen, m_game.editChessboard())) << fen; }

    u64 boardHash() const { return zobrist::computeBoardHash(m_game.readChessboard()); }

    PackedMove findMove(Square source, Square target, PieceType promoteTo = PieceType::NONE)
    {
        PackedMove found = PackedMove::NullMove();
        auto match = [&](PackedMove move) {
            if (move.sourceSqr() != source || move.targetSqr() != target)
                return;
            if (promoteTo == PieceType::NONE ? !move.isPromotion()
                : move.isPromotion() && move.readPromoteToPieceType() == static_cast<i32>(promoteTo))
                found = move;
        };
        if (m_game.readToPlay() == Set::WHITE)
            move_gen_policy::Tusk::forEachMove<Set::WHITE>(m_game.readChessPosition(), match);
        else
            move_gen_policy::Tusk::forEachMove<Set::BLACK>(m_game.readChessPosition(), match);
        EXPECT_NE(PackedMove::NullMove(), found) << "no legal move from " << to_string(source) << " to " << to_string(target);
        return found;
    }

    // makes the move, expects the kept hash to equal a recompute, unmakes it and expects the hash from before.
    void expectHashThroughMakeUnmake(PackedMove move)
    {
        const u64 before = m_game.readChessPosition().hash();
        ASSERT_EQ(boardHash(), before);
        m_game.MakeMove(move);
        EXPECT_EQ(boardHash(), m_game.readChessPosition().hash()) << move.toString() << " made";
        EXPECT_NE(before, m_game.readChessPosition().hash()) << move.toString() << " made";
        m_game.UnmakeMove();
        EXPECT_EQ(before, m_game.readChessPosition().hash()) << move.toString() << " unmade";
    }
};

TEST_F(PositionHashFixture, Hash_EveryPieceOnEverySquare_MatchesComputedAndClearingReturnsToZero)
{
    auto editor = m_position.edit();
    for (ChessPiece piece : c_allPieces) {
        for (u8 index = 0; index < 64; ++index) {
            const Square sqr = static_cast<Square>(index);
            editor.placePiece(piece, sqr);
            ASSERT_NE(0u, editor.hash()) << piece.toString() << to_string(sqr);
            ASSERT_EQ(computedHash(editor), editor.hash()) << piece.toString() << to_string(sqr);
            editor.clearPiece(sqr);
            ASSERT_EQ(0u, editor.hash()) << piece.toString() << to_string(sqr);
        }
    }
}

TEST_F(PositionHashFixture, Hash_EveryPieceOnEverySquare_SevenHundredSixtyEightDistinctKeys)
{
    // a key shared by two piece-squares would make positions that differ by moving one piece hash the same.
    std::set<u64> keys;
    auto editor = m_position.edit();
    for (ChessPiece piece : c_allPieces) {
        for (u8 index = 0; index < 64; ++index) {
            editor.placePiece(piece, static_cast<Square>(index));
            keys.insert(editor.hash());
            editor.clearPiece(static_cast<Square>(index));
        }
    }
    EXPECT_EQ(768u, keys.size());
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
TEST_F(PositionHashFixture, Hash_PlacingThenClearingKiwipete_MatchesComputedAfterEveryEdit)
{
    // setup
    const std::vector<std::pair<ChessPiece, Square>> pieces = {
        { r, A8 }, { k, E8 }, { r, H8 }, { p, A7 }, { p, C7 }, { p, D7 }, { q, E7 }, { p, F7 }, { b, G7 },
        { b, A6 }, { n, B6 }, { p, E6 }, { n, F6 }, { p, G6 }, { P, D5 }, { N, E5 }, { p, B4 }, { P, E4 },
        { N, C3 }, { Q, F3 }, { p, H3 }, { P, A2 }, { P, B2 }, { P, C2 }, { B, D2 }, { B, E2 }, { P, F2 },
        { P, G2 }, { P, H2 }, { R, A1 }, { K, E1 }, { R, H1 } };
    auto editor = m_position.edit();

    // do & validate
    for (const auto& [piece, sqr] : pieces) {
        editor.placePiece(piece, sqr);
        EXPECT_EQ(computedHash(editor), editor.hash()) << "placed " << piece.toString() << to_string(sqr);
    }
    for (size_t i = 0; i < pieces.size(); i += 2) {
        editor.clearPiece(pieces[i].second);
        EXPECT_EQ(computedHash(editor), editor.hash()) << "cleared " << to_string(pieces[i].second);
    }
    for (size_t i = 1; i < pieces.size(); i += 2) {
        editor.clearPiece(pieces[i].first, pieces[i].second);
        EXPECT_EQ(computedHash(editor), editor.hash()) << "cleared " << to_string(pieces[i].second);
    }
    EXPECT_EQ(0u, editor.hash());
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
TEST_F(PositionHashFixture, Hash_ReplacingThroughTheSquareOperatorAndIterator_MatchesComputed)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, E1, n, D4, k, E8);

    // do & validate
    editor[D4] = B;
    EXPECT_EQ(computedHash(editor), editor.hash());

    auto itr = editor.begin();
    itr += static_cast<int>(D4);
    itr.set(r);
    EXPECT_EQ(computedHash(editor), editor.hash());
}

TEST_F(PositionHashFixture, Hash_CastlingProxyGrantsAndRevokes_MatchesComputedAfterEach)
{
    auto editor = m_position.edit();
    auto castling = editor.castling();
    const auto expectInSync = [&](const char* step) { EXPECT_EQ(computedHash(editor), editor.hash()) << step; };

    castling.grantWhiteKingSide();   expectInSync("grant K");
    castling.grantBlackQueenSide();  expectInSync("grant q");
    castling.grantAllWhite();        expectInSync("grant KQ");
    castling.grantBlackKingSide();   expectInSync("grant k");
    castling.revokeWhiteQueenSide(); expectInSync("revoke Q");
    castling.revokeAllBlack();       expectInSync("revoke kq");
    castling.grantAllBlack();        expectInSync("grant kq");
    castling.revokeBlackKingSide();  expectInSync("revoke k");
    castling.revokeWhiteKingSide();  expectInSync("revoke K");
    castling.grantAll();             expectInSync("grant all");
    castling.revokeAllWhite();       expectInSync("revoke KQ");
    castling.revokeBlackQueenSide(); expectInSync("revoke q");
    castling.grantWhiteQueenSide();  expectInSync("grant Q");
    castling.revokeAll();            expectInSync("revoke all");
    castling.grantAll();             expectInSync("grant all again");
    castling.clear();                expectInSync("clear");
    EXPECT_EQ(0u, editor.hash());
}

TEST_F(PositionHashFixture, Hash_CastlingRevokingARightThatIsGone_HashUnchanged)
{
    auto editor = m_position.edit();
    editor.castling().grantAllBlack();
    const u64 hash = editor.hash();

    editor.castling().revokeWhiteKingSide();
    editor.castling().revokeAllWhite();
    editor.castling().grantBlackKingSide();

    EXPECT_EQ(hash, editor.hash());
}

TEST_F(PositionHashFixture, Hash_CastlingWriteFromEveryStateToEveryState_MatchesComputed)
{
    auto editor = m_position.edit();
    for (u8 from = 0; from < 16; ++from) {
        for (u8 to = 0; to < 16; ++to) {
            editor.castling().write(from);
            editor.castling().write(to);
            ASSERT_EQ(to, editor.castling().read());
            ASSERT_EQ(computedHash(editor), editor.hash()) << int(from) << " to " << int(to);
        }
    }
}

TEST_F(PositionHashFixture, Hash_CastlingEveryRight_FourDistinctKeys)
{
    std::set<u64> keys;
    for (u8 right : { CastlingState::WHITE_KINGSIDE, CastlingState::WHITE_QUEENSIDE, CastlingState::BLACK_KINGSIDE, CastlingState::BLACK_QUEENSIDE }) {
        Position position;
        position.edit().castling().write(right);
        keys.insert(position.read().hash());
    }
    EXPECT_EQ(4u, keys.size());
    EXPECT_EQ(0u, keys.count(0));
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ p ][ P ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionHashFixture, Hash_EnPassantWriteSquareOverAnother_TheOldSquareIsHashedOut)
{
    // setup
    auto editor = m_position.edit();
    editor.placePieces(K, E1, P, E4, p, D4, k, E8);
    const u64 withoutEnPassant = editor.hash();

    // do & validate
    editor.enPassant().writeSquare(D3);
    editor.enPassant().writeSquare(E3);
    EXPECT_EQ(computedHash(editor), editor.hash());

    editor.enPassant().clear();
    EXPECT_EQ(withoutEnPassant, editor.hash());
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
TEST_F(PositionHashFixture, Hash_EnPassantClearWithNoneSet_HashUnchanged)
{
    auto editor = m_position.edit();
    editor.placePieces(K, E1, k, E8);
    const u64 hash = editor.hash();

    editor.enPassant().clear();

    EXPECT_EQ(hash, editor.hash());
    EXPECT_FALSE(editor.enPassant());
}

TEST_F(PositionHashFixture, Hash_EnPassantRawWriteOfEverySquare_MatchesComputed)
{
    auto editor = m_position.edit();
    for (Square sqr : { A3, B3, C3, D3, E3, F3, G3, H3, A6, B6, C6, D6, E6, F6, G6, H6 }) {
        EnPassantStateInfo state{};
        state.writeSquare(sqr);
        editor.enPassant().write(state.read());
        EXPECT_EQ(sqr, editor.enPassant().readSquare());
        EXPECT_EQ(computedHash(editor), editor.hash()) << to_string(sqr);
    }
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionHashFixture, Hash_EnPassantRawWriteOfNone_RemovesTheSquareFromTheHash)
{
    // setup, the null move restores the en passant state with a raw write.
    auto editor = m_position.edit();
    editor.placePieces(K, E1, k, E8);
    const u64 withoutEnPassant = editor.hash();
    editor.enPassant().writeSquare(C6);

    // do
    editor.enPassant().write(0);

    // validate
    EXPECT_FALSE(editor.enPassant());
    EXPECT_EQ(withoutEnPassant, editor.hash());

    // and a raw write of none over none changes nothing.
    editor.enPassant().write(0);
    EXPECT_EQ(withoutEnPassant, editor.hash());
}

TEST_F(PositionHashFixture, Hash_EnPassantEveryFile_EightDistinctKeys)
{
    std::set<u64> keys;
    for (Square sqr : { A3, B3, C3, D3, E3, F3, G3, H3 }) {
        Position position;
        position.edit().enPassant().writeSquare(sqr);
        keys.insert(position.read().hash());
    }
    EXPECT_EQ(8u, keys.size());
    EXPECT_EQ(0u, keys.count(0));
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
TEST_F(PositionHashFixture, Hash_CastlingEveryWay_MakeAndUnmakeMatchComputed)
{
    setup("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    expectHashThroughMakeUnmake(findMove(E1, G1));
    expectHashThroughMakeUnmake(findMove(E1, C1));

    setup("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
    expectHashThroughMakeUnmake(findMove(E8, G8));
    expectHashThroughMakeUnmake(findMove(E8, C8));
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
TEST_F(PositionHashFixture, Hash_RookAndKingMovesLosingRights_MakeAndUnmakeMatchComputed)
{
    setup("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    expectHashThroughMakeUnmake(findMove(H1, H5));  // loses K
    expectHashThroughMakeUnmake(findMove(A1, A5));  // loses Q
    expectHashThroughMakeUnmake(findMove(E1, E2));  // loses KQ
    expectHashThroughMakeUnmake(findMove(A1, A8));  // takes a8, loses Q and black's q
    expectHashThroughMakeUnmake(findMove(H1, H8));  // takes h8, loses K and black's k
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ p ][ P ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionHashFixture, Hash_EnPassantCaptureByWhite_MakeAndUnmakeMatchComputed)
{
    setup("4k3/8/8/2pP4/8/8/8/4K3 w - c6 0 1");
    ASSERT_TRUE(m_game.readChessPosition().enPassant());
    const PackedMove capture = findMove(D5, C6);
    ASSERT_TRUE(capture.isEnPassant());
    expectHashThroughMakeUnmake(capture);
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ P ][ p ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ x ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionHashFixture, Hash_EnPassantCaptureByBlack_MakeAndUnmakeMatchComputed)
{
    setup("4k3/8/8/8/5Pp1/8/8/4K3 b - f3 0 1");
    const PackedMove capture = findMove(G4, F3);
    ASSERT_TRUE(capture.isEnPassant());
    expectHashThroughMakeUnmake(capture);
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ p ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ P ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionHashFixture, Hash_DoublePushNextToAnEnemyPawn_MakeAndUnmakeMatchComputed)
{
    // the push records a3 for black's b4 pawn, which goes into the hash.
    setup("4k3/8/8/8/1p6/8/P7/4K3 w - - 0 1");
    const PackedMove push = findMove(A2, A4);
    const u64 before = m_game.readChessPosition().hash();
    m_game.MakeMove(push);
    EXPECT_EQ(A3, m_game.readChessPosition().enPassant().readSquare());
    EXPECT_EQ(boardHash(), m_game.readChessPosition().hash());
    m_game.UnmakeMove();
    EXPECT_FALSE(m_game.readChessPosition().enPassant());
    EXPECT_EQ(before, m_game.readChessPosition().hash());
}

// 8 [ x ][ r ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ P ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionHashFixture, Hash_PromotionToEveryPiece_MakeAndUnmakeMatchComputed)
{
    // x: a8, the quiet promotion; the promotion on b8 takes the rook.
    setup("1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    for (PieceType promoteTo : { PieceType::QUEEN, PieceType::ROOK, PieceType::BISHOP, PieceType::KNIGHT }) {
        expectHashThroughMakeUnmake(findMove(A7, A8, promoteTo));
        expectHashThroughMakeUnmake(findMove(A7, B8, promoteTo));
    }
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ r ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ P ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionHashFixture, Hash_PromotionTakingARookOnItsHomeSquare_RemovesTheRightAndMatchesComputed)
{
    // setup
    setup("4k2r/6P1/8/8/8/8/8/4K3 w k - 0 1");
    const PackedMove promotion = findMove(G7, H8, PieceType::QUEEN);

    // do
    m_game.MakeMove(promotion);

    // validate
    EXPECT_FALSE(m_game.readChessPosition().castling().hasBlackKingSide());
    EXPECT_EQ(boardHash(), m_game.readChessPosition().hash());
    m_game.UnmakeMove();
    EXPECT_TRUE(m_game.readChessPosition().castling().hasBlackKingSide());
    EXPECT_EQ(boardHash(), m_game.readChessPosition().hash());
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ p ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ N ][ x ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionHashFixture, Hash_BlackPromotions_MakeAndUnmakeMatchComputed)
{
    // x: c1, the quiet promotion; the promotion on b1 takes the knight.
    setup("4k3/8/8/8/8/8/2p5/1N2K3 b - - 0 1");
    for (PieceType promoteTo : { PieceType::QUEEN, PieceType::KNIGHT }) {
        expectHashThroughMakeUnmake(findMove(C2, C1, promoteTo));
        expectHashThroughMakeUnmake(findMove(C2, B1, promoteTo));
    }
}

} // namespace ElephantTest
