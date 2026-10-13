#pragma once
// Move generator tests, parameterized on the generator. A fixture deriving from ::testing::TestWithParam<Generator> and
// instantiated with ELEPHANT_MOVE_GEN_AB runs every TEST_P once per generator, today only tusk::MoveGenerator, which
// the search uses. A new generator is A/B tested by adding it to Generator, the helpers & the macro. The helpers only
// look at which moves come out, never how a generator produces them.
#include <gtest/gtest.h>

#include <move/generation/move_gen_policy.hpp>
#include <move/generation/tusk/check_info.hpp>
#include <position/position_accessors.hpp>

namespace ElephantTest {

enum class Generator { Tusk };

// every legal move of us in position, with its capture, castling, en passant & promotion flags.
template<Set us, typename F>
void forEachLegalMove(Generator generator, PositionReader position, F&& onMove)
{
    switch (generator) {
    case Generator::Tusk:
        move_gen_policy::Tusk::forEachMove<us>(position, onMove);
        return;
    }
}

template<Set us>
bool isChecked(Generator generator, PositionReader position)
{
    switch (generator) {
    case Generator::Tusk:
        return tusk::CheckInfo<us>(position).isChecked();
    }
    return false;
}

inline std::string generatorName(const ::testing::TestParamInfo<Generator>& info)
{
    switch (info.param) {
    case Generator::Tusk:
        return "Tusk";
    }
    return "Unknown";
}

} // namespace ElephantTest

#define ELEPHANT_MOVE_GEN_AB(Fixture)                                                                                 \
    INSTANTIATE_TEST_SUITE_P(Generators, Fixture,                                                                      \
        ::testing::Values(::ElephantTest::Generator::Tusk),                                                            \
        ::ElephantTest::generatorName)
