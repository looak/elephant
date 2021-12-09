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
    ChessboardTile emptyTile;
    EXPECT_EQ(emptyTile, b.getTile("a8"));
}

////////////////////////////////////////////////////////////////

} // namespace ElephantTest