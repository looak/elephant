// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2026  Alexander Loodin Ek

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
 * @file input.hpp
 * @brief phant-indexer's input, PGN as it is or compressed: bz2 as OpenBench uploads a workload, gzip. Told apart by
 * the bytes they start with, so standard input needs no option.
 * @author Alexander Loodin Ek  */
#pragma once
#include <string>
#include <string_view>

#include <system/platform.hpp>

namespace indexer {

enum class Compression : u8 { None, Bzip2, Gzip };

// bz2's "BZh" and a block size, gzip's 1f 8b, anything else is taken as plain PGN.
Compression detect(std::string_view bytes);

// Decompresses all of the input, every stream of it when streams are concatenated, into out. False with the reason
// in error when it doesn't decompress or ends early.
bool decompress(std::string_view bytes, Compression compression, std::string& out, std::string& error);

} // namespace indexer
