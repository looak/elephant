#include <gtest/gtest.h>

#include <eval/pesto_tables.hpp>
#include <eval/tapered_score.hpp>

namespace ElephantTest {

////////////////////////////////////////////////////////////////

TEST(TaperedScore, PackAndUnpack_BothHalvesAnySign) {
    for (i32 mg : { 0, 1, -1, 82, -167, 1025, 32767, -32768 }) {
        for (i32 eg : { 0, 1, -1, 94, -99, 936, 32767, -32768 }) {
            const TaperedScore s = S(mg, eg);
            EXPECT_EQ(mg, mg_score(s)) << mg << " " << eg;
            EXPECT_EQ(eg, eg_score(s)) << mg << " " << eg;
        }
    }
}

TEST(TaperedScore, Arithmetic_WorksOnBothHalves) {
    static_assert(S(3, -4) + S(-10, 20) == S(-7, 16));
    static_assert(S(3, -4) - S(-10, 20) == S(13, -24));
    static_assert(-S(3, -4) == S(-3, 4));
    static_assert(S(3, -4) * 3 == S(9, -12));
    static_assert(-2 * S(3, -4) == S(-6, 8));

    TaperedScore sum = S(0, 0);
    sum += S(100, -200);
    sum -= S(300, -50);
    EXPECT_EQ(-200, mg_score(sum));
    EXPECT_EQ(-150, eg_score(sum));
}

TEST(TaperedScore, IntermediateSumsMayLeaveTheRange_TheResultIsExact) {
    // past i16 on the way, back in range at the end: the halves are still right.
    TaperedScore sum = S(0, 0);
    for (int i = 0; i < 10; ++i)
        sum += S(30000, -30000);
    for (int i = 0; i < 10; ++i)
        sum -= S(29990, -29990);
    EXPECT_EQ(100, mg_score(sum));
    EXPECT_EQ(-100, eg_score(sum));
}

TEST(TaperedScore, Taper_MidgameAtFullPhase_EndgameAtZero_LinearBetween) {
    constexpr TaperedScore value = S(100, 300);
    static_assert(evaluator_data::taper(value, evaluator_data::maxGamePhase) == 100);
    static_assert(evaluator_data::taper(value, 0) == 300);
    static_assert(evaluator_data::taper(value, evaluator_data::maxGamePhase / 2) == 200);
    // promotions push the phase past the maximum, it's capped there.
    static_assert(evaluator_data::taper(value, evaluator_data::maxGamePhase + 4) == 100);
    static_assert(evaluator_data::taper(S(-100, -300), 6) == -250);
}

TEST(TaperedScore, PestoTables_AsPublished) {
    // spot checks against chessprogramming.org's PeSTO tables, A8 at index 0.
    static_assert(evaluator_data::pestoMaterial[4] == S(1025, 936));
    static_assert(evaluator_data::pestoPawn[8] == S(98, 178));          // a7
    static_assert(evaluator_data::pestoKnight[0] == S(-167, -58));      // a8
    static_assert(evaluator_data::pestoKing[63] == S(14, -43));         // h1
    static_assert(evaluator_data::pestoTables[3] == evaluator_data::pestoRook);
}

}  // namespace ElephantTest
