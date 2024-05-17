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
#include "king_pin_threats.hpp"
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
}  // namespace pieceFlags

namespace MoveGeneratorFlags {
constexpr u8 silent = 1 << 1;
constexpr u8 capture = 1 << 2;
constexpr u8 all = silent | capture;
}  // namespace MoveGeneratorFlags

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
    void forEachMove(std::function<void(const PrioratizedMove&)> func) const;
    void generate();

    bool isChecked() const;

private:
    void initializeMoveGenerator(PieceType ptype, MoveTypes mtype);

    template<Set set, bool captures>
    void initializeMoveMasks(MaterialMask& target, PieceType ptype);

    template<Set set>
    PackedMove generateNextMove();

    template<Set set>
    void generateAllMoves();

    template<Set set, u8 pieceId>
    void generateMoves(const KingPinThreats& pinThreats);

    template<Set set>
    void internalGenerateMoves(u8 pieceId, const KingPinThreats& pinThreats);

    template<Set set>
    void internalGeneratePawnMoves(const KingPinThreats& pinThreats);
    void internalBuildPawnPromotionMoves(PackedMove move, const KingPinThreats& pinThreats, i32 dstSqr);
    template<Set set>
    void internalGenerateKnightMoves(const KingPinThreats& pinThreats);
    template<Set set>
    void internalGenerateBishopMoves(const KingPinThreats& pinThreats);
    template<Set set>
    void internalGenerateRookMoves(const KingPinThreats& pinThreats);
    template<Set set>
    void internalGenerateQueenMoves(const KingPinThreats& pinThreats);
    template<Set set>
    void internalGenerateKingMoves();

    void genPackedMovesFromBitboard(u8 pieceId, Bitboard movesbb, i32 srcSqr, bool capture, const KingPinThreats& pinThreats);

    Set m_toMove;
    const Position& m_position;

    bool m_movesGenerated;
    uint16_t m_moveCount;
    uint16_t m_currentMoveIndx;
    std::array<PrioratizedMove, 256> m_movesBuffer;

    // pseudo legal move masks for each piece type
    MaterialMask m_moveMasks[2];
    KingPinThreats m_pinThreats[2];
};

template<Set set, u8 pieceId>
void
MoveGenerator::generateMoves(const KingPinThreats& pinThreats)
{
    switch (pieceId) {
        case pawnId:
            internalGeneratePawnMoves<set>(pinThreats);
            break;
        case knightId:
            internalGenerateKnightMoves<set>(pinThreats);
            break;
        case bishopId:
            internalGenerateBishopMoves<set>(pinThreats);
            break;
        case rookId:
            internalGenerateRookMoves<set>(pinThreats);
            break;
        case queenId:
            internalGenerateQueenMoves<set>(pinThreats);
            break;
        case kingId:
            internalGenerateKingMoves<set>();
            break;

        default:
            FATAL_ASSERT(false) << "Invalid piece id";
    }
}

#endif  // MOVE_GENERATOR_HEADER