#include <gtest/gtest.h>
#include "chessboard.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class ChessboardFixture : public ::testing::Test
{
public:
    /*   virtual void SetUp()
    {

    };
    virtual void TearDown() {};
*/
};
////////////////////////////////////////////////////////////////
TEST_F(ChessboardFixture, Empty)
{
    Chessboard b; // by default a board should start empty.
    ChessPiece expectedPiece; // empty, default, 0;

    EXPECT_EQ(expectedPiece, b.readTile({ 0, 0 }));
    EXPECT_EQ(expectedPiece, b.readTile({ 7, 7 }));
    EXPECT_EQ(expectedPiece, b.readTile({ 3, 3 }));
    EXPECT_EQ(expectedPiece, b.readTile({ 4, 4 }));

    EXPECT_EQ(expectedPiece, b.readTile(Notation::BuildPosition('a', 1)));
    EXPECT_EQ(expectedPiece, b.readTile(Notation::BuildPosition('b', 2)));
    EXPECT_EQ(expectedPiece, b.readTile(Notation::BuildPosition('e', 4)));
    EXPECT_EQ(expectedPiece, b.readTile(Notation::BuildPosition('f', 4)));
}

TEST_F(ChessboardFixture, Notation_BuildPosition)
{
    Notation expected(0, 0);
    auto pos = Notation::BuildPosition('a', 1);
    EXPECT_EQ(expected, pos);

    expected = Notation(0xf, 0xf);
    pos = Notation::BuildPosition('z', 1); // invalid position
    EXPECT_EQ(expected, pos);

    Notation defaultValue;
    EXPECT_EQ(expected, defaultValue);

    expected = Notation(4, 3);
    pos = Notation::BuildPosition('e', 4);
    EXPECT_EQ(expected, pos);
}

////////////////////////////////////////////////////////////////

} // namespace ElephantTest