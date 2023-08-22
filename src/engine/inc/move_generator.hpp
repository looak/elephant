// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2023  Alexander Loodin Ek

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
#ifndef MOVE_GENERATOR_HEADER
#define MOVE_GENERATOR_HEADER

#include <queue>
#include "bitboard.h"
#include "move.h"

class GameContext;

typedef std::priority_queue<PrioratizedMove, std::vector<PrioratizedMove>, PrioratizedMoveComparator> PriorityMoveQueue;

class MoveGenerator {
public:
    MoveGenerator(const GameContext& context);
    ~MoveGenerator() = default;

    PackedMove generateNextMove();

private:
    void initializeMoveGenerator();
    template<Set set>
    void initializeMoveMasks(MaterialMask& target);

    template<Set set>
    PackedMove generateNextMove();

    template<Set set, u8 pieceId>
    void generateMoves();

    template<Set set>
    void internalGeneratePawnMoves();
    template<Set set>
    void internalGenerateKnightMoves();
    template<Set set>
    void internalGenerateBishopMoves();
    template<Set set>
    void internalGenerateRookMoves();
    template<Set set>
    void internalGenerateQueenMoves();
    template<Set set>
    void internalGenerateKingMoves();

    const GameContext& m_context;
    PriorityMoveQueue m_moves;
    std::vector<PackedMove> m_returnedMoves;

    // pseudo legal move masks for each piece type
    MaterialMask m_moveMasks[2];
    bool m_movesGenerated;
};

template<Set set, u8 pieceId>
void
MoveGenerator::generateMoves()
{
    switch (pieceId) {
        case pawnId:
            internalGeneratePawnMoves<set>();
            break;
        case knightId:
            internalGenerateKnightMoves<set>();
            break;
        case bishopId:
            internalGenerateBishopMoves<set>();
            break;
        case rookId:
            internalGenerateRookMoves<set>();
            break;
        case queenId:
            internalGenerateQueenMoves<set>();
            break;
        case kingId:
            internalGenerateKingMoves<set>();
            break;

        default:
            FATAL_ASSERT(false) << "Invalid piece id";
    }
}

#endif  // MOVE_GENERATOR_HEADER