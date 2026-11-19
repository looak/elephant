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
 * @file write_tables.hpp
 * @brief Writes tuned values back as code: the engine's pesto_tables.hpp with the numbers inside its tables replaced
 * and everything else, comments & layout, kept. Written with the values it was read with, the file comes out
 * unchanged, the check that the writer is right.    */

#pragma once

#include <string>
#include <vector>

#include <eval/tapered_score.hpp>

namespace tuner {

/**
 * header is the text of pesto_tables.hpp. Returns it with the bodies of pestoMaterial and the six piece tables
 * replaced by values (params:: layout), formatted the way the file has them. Throws when a table isn't found.    */
std::string rewritePestoTables(const std::string& header, const std::vector<TaperedScore>& values);

/**
 * header is the text of mobility_tables.hpp. Returns it with the bodies of the four mobility tables replaced by
 * values (params:: layout), eight to a line as the file has them. Throws when a table isn't found.    */
std::string rewriteMobilityTables(const std::string& header, const std::vector<TaperedScore>& values);

/**
 * header is the text of pawn_tables.hpp. Returns it with the bodies of the ten pawn tables replaced by values
 * (params:: layout), one line each as the file has them. Throws when a table isn't found.    */
std::string rewritePawnTables(const std::string& header, const std::vector<TaperedScore>& values);

/**
 * header is the text of king_tables.hpp. Returns it with the bodies of kingShelter, kingStorm, kingStormBlocked and
 * kingDanger replaced by values (params:: layout), eight to a line as the file has them. Throws when a table isn't found.    */
std::string rewriteKingTables(const std::string& header, const std::vector<TaperedScore>& values);

/**
 * header is the text of piece_tables.hpp. Returns it with the body of piecePair replaced by values (params:: layout),
 * one line as the file has it. Throws when a table isn't found.    */
std::string rewritePieceTables(const std::string& header, const std::vector<TaperedScore>& values);

} // namespace tuner
