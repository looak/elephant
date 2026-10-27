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
 *     <fen> 1-0                        a bare result as the last token, also 1/2-1/2, 0-1, 1.0, 0.5, 0.0, or any number
 *                                      from 0 to 1, the average of several games' results
 *
 * The FEN needs its first four fields, the halfmove & fullmove counters are optional. Results are always from white's
 * side: 1.0 white won, 0.5 draw, 0.0 black won. Empty lines and lines starting with # are skipped.
 *
 * A file is read in byte ranges that each start on a line, so threads can read their own part of it, see
 * splitDataset.    */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tuner {

struct TuningPosition {
    std::string fen;
    double result;  // white's side, 1.0 win, 0.5 draw, 0.0 loss
};

struct LoadError {
    std::size_t line;  // 1-based, within the range the report covers until reports are appended
    std::string reason;
};

struct LoadReport {
    static constexpr std::size_t c_maxErrors = 5;

    std::size_t lines = 0;
    std::size_t skipped = 0;     // empty & comment lines
    std::size_t noResult = 0;    // a line without a result in a known form
    std::size_t badFen = 0;      // the engine's FEN parser refused it
    std::vector<LoadError> firstErrors;  // a few of the refused lines

    // adds the report of the range that follows this one's, its line numbers move on by this report's lines.
    void append(const LoadReport& next);
};

// [begin, end) in bytes, begin at the start of a line.
struct ByteRange {
    std::uint64_t begin = 0;
    std::uint64_t end = 0;
};

/**
 * Splits a dataset file into count ranges of about equal size, each starting on a line, together covering its first
 * limit positions (0 for the whole file). Fewer ranges for a file too small to split. Throws when the file can't be
 * opened.    */
std::vector<ByteRange> splitDataset(const std::string& path, std::size_t count, std::size_t limit);

/**
 * Splits a dataset line into its FEN & result, nullopt when no result is found. Doesn't check the FEN.    */
std::optional<TuningPosition> parseLine(std::string_view line);

/**
 * Streams the positions in range of a dataset file, in order, without keeping them. handle returns false for a
 * position it couldn't use (an invalid FEN), counted in report.badFen. Line numbers in report count from the range's
 * start. Throws when the file can't be opened.    */
void forEachPosition(const std::string& path, ByteRange range, LoadReport& report,
                     const std::function<bool(const TuningPosition&)>& handle);

/**
 * Reads every position of a dataset file into memory, each FEN checked with the engine's FEN parser. For small
 * files, a large one is better streamed with forEachPosition.    */
std::vector<TuningPosition> loadDataset(const std::string& path, LoadReport& report);

}  // namespace tuner
