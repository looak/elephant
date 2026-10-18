// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2025  Alexander Loodin Ek

// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.If not, see < http://www.gnu.org/licenses/>.

/**
 * @file fit.hpp
 * @brief Phase 2 of tuning: changing the evaluation's values until it predicts the games' results as well as it can.
 *
 * What is being changed
 * ---------------------
 * The values are the numbers in the evaluation: PeSTO's material and its piece square tables, and mobility's tables,
 * each a midgame and an endgame number (the two halves of S(mg, eg)). 456 values, 912 numbers, in three groups that
 * can each be tuned or left as they are (see What's kept fixed). Phase 0 recorded, for every position, which
 * values it uses and how often (its coefficients), so a position's evaluation can be recomputed from any set of
 * values without the engine:
 *
 *     eval = sum over its values of  coefficient * (mg * phase / 24  +  eg * (24 - phase) / 24)  +  offset
 *
 * phase is how much material is left, 24 with everything on the board and 0 with only kings & pawns. So a value's
 * midgame number counts in positions full of pieces, its endgame number in positions without, and in between both
 * count a share. offset is what the evaluation adds that isn't tuned (mop-up), it's carried along unchanged.
 *
 * What is being minimised
 * -----------------------
 * The same error as in phase 1 (find_k.hpp): each position's evaluation becomes an expected result with
 * expected = 1 / (1 + 10^(-K * eval / 400)), K fixed at what phase 1 found, and the error is the mean of
 * (result - expected)^2 over every position. Lower is better, phase 1's error is where it starts.
 *
 * How the values move: the gradient
 * ---------------------------------
 * For each number the question is: if it went up a little, would the error go up or down, and how much? That's its
 * gradient, worked out position by position and added up:
 *
 *   1. miss = result - expected. Positive: white did better than the evaluation predicted, the evaluation was too
 *      pessimistic for white. Negative: too optimistic.
 *   2. slope = how much expected moves when eval moves a centipawn: expected * (1 - expected) * K * ln(10) / 400.
 *      Steep near 50%, flat near 0% or 100%: a position the evaluation already calls won hardly moves the prediction.
 *   3. The error of this position, (result - expected)^2, changes by -2 * miss * slope per centipawn of eval.
 *   4. A centipawn of eval comes from the numbers in proportion to how much each counts in this position:
 *      coefficient * phase / 24 for a midgame number, coefficient * (24 - phase) / 24 for an endgame number.
 *
 * So each number's gradient is the sum over its positions of -2 * miss * slope * coefficient * (its phase share).
 * Moving every number a little against its gradient lowers the error. Doing that again and again is gradient descent;
 * each round over every position is a pass (an "epoch").
 *
 * How far to move: Adam
 * ---------------------
 * Gradients are tiny for a square that's rarely used and large for material, the same step size can't suit both.
 * Adam keeps, per number, a running average of its gradient (which way it's been pushed lately) and of the gradient
 * squared (how hard), and steps by roughly rate * average / sqrt(average of squares). Every number then moves about
 * `rate` centipawns a pass while its push is steady, and less once the pushes start to disagree, near its best.
 *
 * What's kept fixed
 * -----------------
 * Only the groups asked for move (FitSettings, --tune), the others stay as they are but still count in every
 * position's evaluation. That's how a new term joins an evaluation the search is used to: with PeSTO's tables fixed,
 * mobility's values are fitted alongside them and only take what PeSTO doesn't already say.
 *
 * Mobility's tables are kept centred: after every step each piece's table is moved up or down so its average, weighted
 * by how often each count occurs in the positions, is 0. Every piece has exactly one mobility value, so a table's
 * average works like the piece's material value, and left free the fit uses it to change what pieces are worth
 * (it lowered every piece's midgame value by ~60 that way, the kind of shift that cost Elo in the tables' retune).
 * Centred, mobility only says how much a free piece is worth over a hemmed in one. --free-mobility turns this off.
 *
 * Material is never tuned together with the piece square tables by default: adding the same amount to every square of
 * a piece's table is the same as adding it to the piece's material, the two can trade values forever without the
 * error changing. Squares no position uses (pawns on the first & last rank, a knight with 9 safe squares) get no
 * gradient and keep their value.    */

#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include <eval/tapered_score.hpp>
#include <trace.hpp>
#include <tuner_data.hpp>

namespace tuner {

// the values being tuned, in params:: layout, as real numbers while they're tuned. Rounded when written out.
struct Values {
    std::vector<double> mg;
    std::vector<double> eg;

    static Values from(const std::vector<TaperedScore>& scores);
    // rounded to whole centipawns, the form the engine keeps them in.
    [[nodiscard]] std::vector<TaperedScore> rounded() const;
};

struct FitSettings {
    double k = 1.0;             // phase 1's K, fixed
    std::size_t epochs = 1000;  // passes over every position
    double rate = 1.0;          // Adam's step, about how many centipawns a number moves per pass at most
    // which groups of values move, a bit per params::Group, the others stay as they are, see "What's kept fixed" above.
    u32 tuned = bit(params::Group::Psqt) | bit(params::Group::Mobility);

    // mobility tables kept centred, their usage weighted average at 0, see "What's kept fixed" above.
    bool centreMobility = true;

    [[nodiscard]] bool tunes(params::Group group) const {
        return (tuned & bit(group)) != 0;
    }
    std::size_t threads = 1;
};

// the error of the entries with these values at this K, phase 1's error with values instead of the static eval.
[[nodiscard]] double fitError(const TunerData& data, const Values& values, double k, std::size_t threads);

/**
 * Runs settings.epochs passes of gradient descent with Adam from values, see above, and leaves the tuned values in
 * values. After every pass, progress(epoch, error before that pass) is called.    */
void fit(const TunerData& data, Values& values, const FitSettings& settings,
         const std::function<void(std::size_t epoch, double error)>& progress);

} // namespace tuner
