/******************************************************************************
 * Elephant Gambit Chess Engine - a Chess AI
 * Copyright(C) 2025  Alexander Loodin Ek
 * 
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < http://www.gnu.org/licenses/>. 
 *****************************************************************************/

#pragma once
#include <cstdlib>
#include <cstring>
#include <string>

namespace ephant {
// file name without its directories, cut at the last '/' or '\'.
constexpr const char* file_name(const char* path) {
    const char* name = path;
    for (const char* c = path; *c != '\0'; ++c)
        if (*c == '/' || *c == '\\')
            name = c + 1;
    return name;
}

class ephsert final {
public:
    // logs the failure as critical, see assert.cpp: the log is in logger.cpp, not in every file that asserts.
    ephsert(char const* expression, const std::string& prefix, const std::string& file, int line, const std::string& message = "");

    ~ephsert() {
        abort();
    }
};

}  // namespace ephant

#define __FILENAME__ (ephant::file_name(__FILE__))

// Asserts are for engine invariants and engine failures, bad input (FEN, SAN/PGN, UCI, CLI) throws, see exceptions.hpp.

// @brief Asserts that the expression evaluates to true and logs a fatal assert message with the expression, file name and line
// number if it fails.
#if defined(ASSERTIONS_ENABLED)
#define ASSERT_MSG(expr, message) \
    if ((expr) != 0) {     \
        int ___noop = 5;   \
        (void)___noop;     \
    }                      \
    else                   \
        ephant::ephsert(#expr, "[ASSERT] ", __FILENAME__, __LINE__, message);
#else
#define ASSERT_MSG(expr, message) \
    switch (0)    \
    case 0:       \
    default:      \
        ;
#endif

#if defined(ASSERTIONS_ENABLED)
#define ASSERT(expr) \
    if ((expr) != 0) {     \
        int ___noop = 5;   \
        (void)___noop;     \
    }                      \
    else                   \
        ephant::ephsert(#expr, "[ASSERT] ", __FILENAME__, __LINE__);
#else
#define ASSERT(expr) \
    switch (0)    \
    case 0:       \
    default:      \
        ;
#endif