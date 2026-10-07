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
#include <type_traits>
#include <utility>
#include <diagnostics/assert.hpp>
#include <core/square.hpp>

template <typename dst, typename src>
constexpr dst checked_cast(src value) {
    static_assert(std::is_integral_v<src>, "checked_cast source must be integral");
    static_assert(std::is_integral_v<dst>, "checked_cast destination must be integral");

    ASSERT_MSG(std::in_range<dst>(value), "Cast data loss detected!");

    return static_cast<dst>(value);
}

template<typename T>
constexpr T&
increment(T& value) {
    static_assert(std::is_integral<std::underlying_type_t<T>>::value, "Can't increment value");
    ((std::underlying_type_t<T>&)value)++;
    return value;
}