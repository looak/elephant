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

#include <map>
#include <vector>
#include <functional>
#include <optional>
#include "move.h"
#include "defines.h"

class GameContext;
struct MoveCount
{
    typedef std::function<bool(const Move&)> Predicate;
    void operator += (const MoveCount& rhs)
    {
        this->Captures 		+= rhs.Captures;
        this->Promotions 	+= rhs.Promotions;
        this->EnPassants 	+= rhs.EnPassants;
        this->Castles 		+= rhs.Castles;
        this->Checks 		+= rhs.Checks;
        this->Checkmates 	+= rhs.Checkmates;
        this->Moves 		+= rhs.Moves;
    }
    u32 Captures = 0;
    u32 Promotions = 0;
    u32 EnPassants = 0;
    u32 Castles = 0;
    u32 Checks = 0;
    u32 Checkmates = 0;
    u32 Moves = 0;
};

struct PieceKey
{
    ChessPiece Piece;
    Notation SourceSqr;

    bool operator<(const PieceKey& rhs) const 
    { 
        if (Piece == rhs.Piece)
            return SourceSqr < rhs.SourceSqr;
        
        return Piece < rhs.Piece;
    }
};

class MoveGenerator
{
public:
    std::vector<Move> GeneratePossibleMoves(const GameContext& context) const;

    MoveCount CountMoves(const std::vector<Move>& moves, MoveCount::Predicate predicate = [](const Move&) { return true; }) const;
    std::vector<std::string> MoveAnnotations(const std::vector<Move>& moves, MoveCount::Predicate predicate = [](const Move&) { return true; }) const;
    std::map<PieceKey, std::vector<Move>> OrganizeMoves(const std::vector<Move>& moves) const;

};