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
 * @file find_k.hpp
 * @brief Phase 1 of tuning: finding K, the scale between the evaluation and the game's result.
 *
 * The problem K solves
 * --------------------
 * The tuner wants to know how well the evaluation predicts who wins. But the two are in different units: the
 * evaluation is in centipawns (+150 = white is a piece-and-a-half better), the result is 1, 0.5 or 0 (white won, drew,
 * lost). To compare them the evaluation is turned into an *expected* result with a sigmoid, an S-shaped curve:
 *
 *     expected(eval) = 1 / (1 + 10^(-K * eval / 400))
 *
 *     eval = 0     -> 0.5, an even position is expected to be drawn on average
 *     eval = +big  -> close to 1, white should win
 *     eval = -big  -> close to 0, black should win
 *
 * K is how steep the S is, how many centipawns it takes to be "probably winning". With a large K, +100 already means
 * a near certain win; with a small K, even +400 is still uncertain. The 400 is the same scale Elo uses, so K = 1 reads
 * like an Elo difference: +400 centipawns ~ a 91% expected score.
 *
 * Measuring how wrong the evaluation is
 * -------------------------------------
 * For every position the error is (result - expected(eval))^2: 0 when the prediction is exact, up to 1 when the
 * evaluation was certain and the other side won. The mean of that over all positions (the mean squared error, MSE)
 * says how well the evaluation predicts results. The lower the better.
 *
 * Why K is found first, and then kept fixed
 * -----------------------------------------
 * The tuner will change the evaluation's values to lower that error. But the error depends on K too, and K and the
 * values can trade places: doubling every value and halving K predicts exactly the same. If both could move, the
 * tuner could inflate or shrink every value without improving anything. So K is found once, for the current values,
 * as the K with the lowest error, and kept. The tuned values then stay on the same centipawn scale as today, which
 * matters because the search's margins (futility, delta pruning) are written in those centipawns.
 *
 * The error at that K is also the baseline: what the current evaluation scores before any tuning, the number every
 * tuned set of values has to beat.    */

#pragma once

#include <cstddef>
#include <vector>

#include <tuner_data.hpp>

namespace tuner {

// the expected result for white of a position white evaluates at eval centipawns, see above.
[[nodiscard]] double expectedResult(double eval, double k);

/**
 * The mean squared error of the entries' static evaluations against their results at this K: the mean of
 * (result - expectedResult(staticEval, k))^2. Split over threads, each sums its own slice of the entries, the slices
 * are added in order so the result doesn't depend on the number of threads.    */
[[nodiscard]] double meanSquaredError(const TunerData& data, double k, std::size_t threads);

struct KSearch {
    double k = 0.0;        // the K with the lowest error
    double error = 0.0;    // the error at that K, the baseline tuning has to beat
    // every K tried and its error, coarse to fine, to show the error's curve.
    std::vector<std::pair<double, double>> tried;
};

/**
 * Finds the K with the lowest error by narrowing in: tries K from 0 to 3 in steps of 0.1, then steps of 0.01 around
 * the best of those, and so on down to steps of 0.0001. The error has one valley in K, a step either side of the
 * best one brackets it.    */
[[nodiscard]] KSearch findK(const TunerData& data, std::size_t threads);

} // namespace tuner
