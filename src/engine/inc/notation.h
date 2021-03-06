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

#pragma once
#include "defines.h"
#include <string>

struct Notation
{
	static Notation BuildPosition(byte file, byte rank);
	static bool Validate(const Notation& notation);
	static std::string toString(const Notation& notation);
	// Should Notation validate rank & file?
	// Should Notation validate format of rank & file?
	Notation() :
		file(0xF),
		rank(0xF)
	{}

	Notation(byte index)
	{
		if (index > 127)
			LOG_ERROR() << "In case index is larger than 127 it will wrap around our board.";
		file = index % 8;
		rank = index / 8;
	}

	Notation(byte _file, byte _rank) :
		file(_file),
		rank(_rank)
	{}

	Notation(Notation&& other) = default;
	explicit Notation(const Notation& other) = default;

	byte file : 4;
	byte rank : 4;

	byte index() const { return (rank * 8) + file; }
	bool operator==(const Notation& rhs) const;
	bool operator!=(const Notation& rhs) const;
	bool operator<(const Notation& rhs) const;
	Notation& operator=(Notation&& other);
};