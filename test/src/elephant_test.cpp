#include "gtest/gtest.h"
#include "static_initializer.hpp"

bool g_initialized = static_initializer::initialize();

int main(int argc, char* argv[])
{
    assert(g_initialized);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}