#include <gtest/gtest.h>

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class BitboardFixture : public ::testing::Test
{
public:
    /*   virtual void SetUp()
    {

    };
    virtual void TearDown() {};
*/
};
////////////////////////////////////////////////////////////////
TEST_F(BitboardFixture, Empty)
{
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}

////////////////////////////////////////////////////////////////

}