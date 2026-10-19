#include <gtest/gtest.h>

#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <move/generation/move_gen_policy.hpp>
#include <search/perft_search.hpp>
#include <perft_cases.hpp>

#include <cctype>
#include <string>

// Every perft position at the deepest depth within c_perftShallowBudget nodes, milliseconds in all. ElephantSuite runs
// the same positions deeper, see perft_cases.hpp.

namespace ElephantTest {

class PerftShallow : public ::testing::TestWithParam<PerftPosition> {};

TEST_P(PerftShallow, NodeCount)
{
    const PerftPosition& position = GetParam();
    const int depth = position.deepestWithin(c_perftShallowBudget);
    ASSERT_GT(depth, 0) << position.name << " is over the shallow budget at depth 1";

    GameContext context;
    io::fen_parser::deserialize(position.fen.c_str(), context.editChessboard());
    PerftSearch perft(context);

    EXPECT_EQ(position.nodesAt(depth), perft.Run<move_gen_policy::Tusk>(depth).Nodes)
        << position.name << " at depth " << depth << "\n  " << position.fen;
}

INSTANTIATE_TEST_SUITE_P(Positions, PerftShallow, ::testing::ValuesIn(perftPositions()),
    [](const ::testing::TestParamInfo<PerftPosition>& param) {
        std::string name = std::to_string(param.index) + "_";
        for (char c : param.param.name)
            name += std::isalnum(static_cast<unsigned char>(c)) ? c : '_';
        return name;
    });

} // namespace ElephantTest
