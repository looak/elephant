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
#include "transposition_table.hpp"
#include "move.h"
#include <position/position.hpp>

class GameContext;
class Search;

enum class MoveTypes {
    ALL,
    CAPTURES_ONLY,
    QUIET_ONLY,
};

namespace move_generator_constants {
// priority values for move generator
// higher value means higher priority
constexpr u16 capturePriority = 1000;
constexpr u16 promotionPriority = 2000;
constexpr u16 checkPriority = 900;
constexpr u16 pvMovePriority = 5000;
constexpr u16 killerMovePriority = 800;
} // namespace move_generator_constants

class MoveGenerator {
public:
    MoveGenerator(const GameContext& context);
    MoveGenerator(const GameContext& context, const TranspositionTable& tt, const Search& search, u32 ply);
    MoveGenerator(const Position& pos, Set toMove, PieceType ptype = PieceType::NONE, MoveTypes mtype = MoveTypes::ALL);
    ~MoveGenerator() = default;

    PrioratizedMove generateNextMove();
    void forEachMove(std::function<void(const PrioratizedMove&)> func) const;
    void generate();

    bool isChecked() const;

    template<Set us>
    const KingPinThreats& readKingPinThreats() const;

    template<Set set>
    const MaterialMask& readMoveMasks() const { return m_moveMasks[static_cast<u8>(set)]; }

private:
    void initializeMoveGenerator(PieceType ptype, MoveTypes mtype);

    template<Set set, bool captures>
    void initializeMoveMasks(MaterialMask& target, PieceType ptype);

    template<Set set>
    PrioratizedMove generateNextMove();

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

    void genPackedMovesFromBitboard(u8 setId, u8 pieceId, Bitboard movesbb, i32 srcSqr, bool capture, const KingPinThreats& pinThreats);

    void sortMoves();

    Set m_toMove;
    const Position& m_position;
    const TranspositionTable* m_tt;
    const Search* m_search;
    const u32 m_ply;
    u64 m_hashKey;

    bool m_movesGenerated;
    uint16_t m_moveCount;
    uint16_t m_currentMoveIndx;
    std::array<PrioratizedMove, 256> m_movesBuffer;  // 1kb

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


template<Set us>
const KingPinThreats& MoveGenerator::readKingPinThreats() const
{
    return m_pinThreats[static_cast<u8>(us)];
}

#endif  // MOVE_GENERATOR_HEADER