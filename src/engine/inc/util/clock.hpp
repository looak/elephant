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

/**
 * @file clock.hpp
 * @brief Provides a Clock class for measuring elapsed time and calculating nodes per second 
 * 
 */

#pragma once

#include <chrono>

#include "defines.hpp"


class Clock
{
public:
    Clock() = default;

    void Start();
    void Stop();

    /**
     * @return elapsed time since Start() in milliseconds represented as a signed 64-bit integer.   */
    i64 getElapsedTime() const;

    u64 calcNodesPerSecond(u64 nodes) const;

private:
    bool m_running = false;
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_endTime;

};