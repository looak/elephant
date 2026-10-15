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
 * @file build_info.hpp
 * @brief What the binary was built from and how, the UCI "buildinfo" command reports it.
 * A test runner uses it to tell whether two runs are comparable: the same commit built by another compiler, or with
 * assertions, can search differently.
 * @author Alexander Loodin Ek  */
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace build_info {

// the full commit hash the binary was built from, empty outside a git checkout.
const std::string& commit();
// the commit's first 7 characters plus "-dirty" when the working tree had changes, "nogit" outside a checkout.
std::string shortCommit();

// key, value pairs in the order "buildinfo" prints them: version, commit, dirty, branch, compiler, config,
// assertions, exceptions, bits. A value never holds a newline, a key never holds a space.
std::vector<std::pair<std::string, std::string>> entries();

}  // namespace build_info
