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
 * @file dataset.hpp
 * @brief The tuner's training data: positions with the result of the game they came from, read from a text file with
 * one position per line, a FEN followed by the result in any of the common forms:
 *
 *     <fen> [1.0]                      lichess-big3 & Ethereal style, also [0.5] & [0.0]
 *     <fen> c9 "1-0";                  zurichess quiet-labeled, also "1/2-1/2" & "0-1", any EPD opcode
 *     <fen> 1-0                        a bare result as the last token, also 1/2-1/2, 0-1, 1.0, 0.5, 0.0
 *
 * The FEN needs its first four fields, the halfmove & fullmove counters are optional. Results are always from white's
 * side: 1.0 white won, 0.5 draw, 0.0 black won. Empty lines and lines starting with # are skipped.    */

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tuner {

struct TuningPosition {
    std::string fen;
    double result;  // white's side, 1.0 win, 0.5 draw, 0.0 loss
};

struct LoadReport {
    std::size_t lines = 0;
    std::size_t skipped = 0;     // empty & comment lines
    std::size_t noResult = 0;    // a line without a result in a known form
    std::size_t badFen = 0;      // the engine's FEN parser refused it
    std::vector<std::string> firstErrors;  // a few of the refused lines, "<line number>: <reason>"
};

/**
 * Splits a dataset line into its FEN & result, nullopt when no result is found. Doesn't check the FEN, see
 * loadDataset.    */
std::optional<TuningPosition> parseLine(std::string_view line);

/**
 * Reads every position of a dataset file, each FEN checked with the engine's FEN parser. Throws when the file can't
 * be opened, lines that can't be used are counted in the report.    */
std::vector<TuningPosition> loadDataset(const std::string& path, LoadReport& report);

}  // namespace tuner
