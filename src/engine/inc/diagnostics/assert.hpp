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
#include <spdlog/spdlog.h>

namespace ephant {
class ephsert final {
public:
    ephsert(char const* expression, const std::string& prefix, const std::string& file, int line, const std::string& message = "")
    {
        spdlog::critical("{}{}:{} Assertion failed: {} > {}", prefix, file, line, expression, message);
    }

    ~ephsert() {
        abort();
    }
};

}  // namespace ephant


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