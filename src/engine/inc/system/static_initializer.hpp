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
 * @file static_initializer.hpp
 * @brief Provides functions to initialize various components of a bitboard chess engine 
 * 
 */

#pragma once

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/rays/rays.hpp>
#include <io/weight_store.hpp>
#include <position/hash_zobrist.hpp>

namespace static_initializer {
bool initialize() {
    attacks::internals::initialize();
    ray::internals::initialize();
    zobrist::internals::initialize();

    assert(zobrist::internals::initialized());

    WeightStore::get()->initialize();
    return true;
}

} // namespace static_initializer