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
#include "move.h"
#include "position.hpp"

class GameContext;

typedef std::priority_queue<PrioratizedMove, std::vector<PrioratizedMove>, PrioratizedMoveComparator> PriorityMoveQueue;

namespace pieceFlags {
constexpr u8 pawns = 1 << 1;
constexpr u8 knights = 1 << 2;
constexpr u8 bishops = 1 << 3;
constexpr u8 rooks = 1 << 4;
constexpr u8 queens = 1 << 5;
constexpr u8 kings = 1 << 6;
constexpr u8 all = pawns | knights | bishops | rooks | queens | kings;
};  // namespace pieceFlags

namespace MoveGeneratorFlags {
constexpr u8 silent = 1 << 1;
constexpr u8 capture = 1 << 2;
constexpr u8 all = silent | capture;
};  // namespace MoveGeneratorFlags

// template<Set us, u8 pieceFlag = pieceFlags::all, u8 moveGenFlags = MoveGeneratorFlags::all>
// class MoveGeneratorOther {
// private:
//     constexpr Set m_them = opposing_set<us>();
// };

enum class MoveTypes {
    ALL,
    CAPTURES_ONLY,
    QUIET_ONLY,
};

class MoveGenerator {
public:
    MoveGenerator(const GameContext& context);
    MoveGenerator(const Position& pos, Set toMove, PieceType ptype = PieceType::NONE, MoveTypes mtype = MoveTypes::ALL);
    ~MoveGenerator() = default;

    PackedMove generateNextMove();
    void forEachMove(std::function<void(const PackedMove)> func) const;
    void generate();

private:
    void initializeMoveGenerator(PieceType ptype, MoveTypes mtype);

    template<Set set>
    void initializeMoveMasks(MaterialMask& target, PieceType ptype, MoveTypes mtype);

    template<Set set>
    PackedMove generateNextMove();

    template<Set set>
    void generateAllMoves();

    template<Set set, u8 pieceId>
    void generateMoves(const KingMask& kingmask);

    template<Set set>
    void internalGenerateMoves(u8 pieceId, const KingMask& kingmask);

    template<Set set>
    void internalGeneratePawnMoves(const KingMask& kingmask);
    template<Set set>
    void internalGenerateKnightMoves(const KingMask& kingmask);
    template<Set set>
    void internalGenerateBishopMoves(const KingMask& kingmask);
    template<Set set>
    void internalGenerateRookMoves(const KingMask& kingmask);
    template<Set set>
    void internalGenerateQueenMoves(const KingMask& kingmask);
    template<Set set>
    void internalGenerateKingMoves(const KingMask& kingmask);

    void genPackedMovesFromBitboard(u8 pieceId, Bitboard movesbb, i32 srcSqr, bool capture, const KingMask& kingmask);

    Set m_toMove;
    const Position& m_position;

    PriorityMoveQueue m_moves;
    std::vector<PackedMove> m_unsortedMoves;
    std::vector<PackedMove> m_returnedMoves;

    // pseudo legal move masks for each piece type
    MaterialMask m_moveMasks[2];
    bool m_movesGenerated;
    KingMask m_kingMask[2];
};

template<Set set, u8 pieceId>
void
MoveGenerator::generateMoves(const KingMask& kingmask)
{
    switch (pieceId) {
        case pawnId:
            internalGeneratePawnMoves<set>(kingmask);
            break;
        case knightId:
            internalGenerateKnightMoves<set>(kingmask);
            break;
        case bishopId:
            internalGenerateBishopMoves<set>(kingmask);
            break;
        case rookId:
            internalGenerateRookMoves<set>(kingmask);
            break;
        case queenId:
            internalGenerateQueenMoves<set>(kingmask);
            break;
        case kingId:
            internalGenerateKingMoves<set>(kingmask);
            break;

        default:
            FATAL_ASSERT(false) << "Invalid piece id";
    }
}

#endif  // MOVE_GENERATOR_HEADER