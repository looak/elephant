// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021  Alexander Loodin Ek

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

// This is all based on the Forsyth–Edwards Notation standard for chess
// board positions. https://en.wikipedia.org/wiki/Forsyth-Edwards_Notation
#pragma once
#include <string>

class GameContext;

class FENParser
{
public:
    static bool deserialize(const char* input, GameContext& outputContext);
    static bool serialize(const GameContext& inputContext, std::string fen);
};