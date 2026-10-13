#pragma once
// A/B tests of the move generators. A fixture deriving from ::testing::TestWithParam<Generator> and instantiated with
// ELEPHANT_MOVE_GEN_AB runs every TEST_P once against the legacy MoveGenerator and once against tusk::MoveGenerator,
// which the search uses. The helpers only look at which moves come out, never how a generator produces them.
#include <gtest/gtest.h>

#include <move/generation/move_gen_policy.hpp>
#include <move/generation/move_generator.hpp>
#include <move/generation/tusk/check_info.hpp>
#include <position/position_accessors.hpp>

namespace ElephantTest {

enum class Generator { Legacy, Tusk };

// every legal move of us in position, with its capture, castling, en passant & promotion flags.
template<Set us, typename F>
void forEachLegalMove(Generator generator, PositionReader position, F&& onMove)
{
    if (generator == Generator::Legacy)
        move_gen_policy::Legacy::forEachMove<us>(position, onMove);
    else
        move_gen_policy::Tusk::forEachMove<us>(position, onMove);
}

template<Set us>
bool isChecked(Generator generator, PositionReader position)
{
    if (generator == Generator::Legacy) {
        MoveGenParams params;
        return MoveGenerator<us>(position, params).isChecked();
    }
    return tusk::CheckInfo<us>(position).isChecked();
}

inline std::string generatorName(const ::testing::TestParamInfo<Generator>& info)
{
    return info.param == Generator::Legacy ? "Legacy" : "Tusk";
}

} // namespace ElephantTest

#define ELEPHANT_MOVE_GEN_AB(Fixture)                                                                                 \
    INSTANTIATE_TEST_SUITE_P(Generators, Fixture,                                                                      \
        ::testing::Values(::ElephantTest::Generator::Legacy, ::ElephantTest::Generator::Tusk),                         \
        ::ElephantTest::generatorName)
