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
#pragma once

#include <array>
#include <queue>
#include <functional>
#include <move_generation/bulk_move_generator.hpp>
#include <move_generation/king_pin_threats.hpp>
#include <move_generation/move_ordering_view.hpp>
#include <position/position_accessors.hpp>


struct MoveGenParams {
    const MoveOrderingView* ordering = nullptr;
    MoveTypes moveFilter = MoveTypes::ALL;
    bool inCheck = false;
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

template<Set _us>
class MoveGenerator {
public:
    explicit MoveGenerator(PositionReader position, const MoveGenParams& params);

    PrioritizedMove generateNextMove();

private:
    enum class Stage {
        PV_MOVE,
        CAPTURES_GEN,
        CAPTURES_SORT,
        KILLERS,
        QUIETS_GEN,
        QUIETS_SORT,
        DONE
    };

    KingPinThreats computeKingPinThreats();
    void internalGenerateMoves();
   
    void internalGeneratePawnMoves(BulkMoveGenerator bulkMoveGen);
    void internalBuildPawnPromotionMoves(PackedMove move, i32 dstSqr);    
    void internalGenerateKnightMoves(BulkMoveGenerator bulkMoveGen);    
    void internalGenerateMovesGeneric(BulkMoveGenerator bulkMoveGen, u8 pieceId);
    // void internalGenerateBishopMoves();    
    // void internalGenerateRookMoves();    
    // void internalGenerateQueenMoves();    
    void internalGenerateKingMoves(BulkMoveGenerator bulkMoveGen);

    void buildPackedMoveFromBitboard(u8 pieceId, Bitboard movesbb, Square srcSqr, bool capture);

    std::tuple<Bitboard, Bitboard> isolatePiece(u8 pieceId, Square source, Bitboard movesbb) const;    
    std::tuple<Bitboard, Bitboard> isolatePawn(Square source, Bitboard movesbb) const;    
    std::tuple<Bitboard, Bitboard> isolateKnight(Square source, Bitboard movesbb) const;    
    std::tuple<Bitboard, Bitboard> isolateBishop(Square source, Bitboard movesbb) const;    
    std::tuple<Bitboard, Bitboard> isolateRook(Square source, Bitboard movesbb) const;

    std::array<PrioritizedMove, 256> m_movesBuffer; // 1kb
    u32 m_currentMoveIndx;
    u32 m_moveCount;
    
    PositionProxy<PositionReadOnlyPolicy> m_position;
    const MoveOrderingView* m_ordering;
    Stage m_stage;
    
    KingPinThreats m_pinThreats[2];

    bool m_movesGenerated;
};


template<Set us>
KingPinThreats MoveGenerator<us>::computeKingPinThreats() {
    constexpr Set op = opposing_set<us>();  
    Square kingSqr = static_cast<Square>(m_position.material().king<us>().lsbIndex());
    Square opKingSqr = static_cast<Square>(m_position.material().king<op>().lsbIndex());

    KingPinThreats ret;
    ret.evaluate<us>(kingSqr, m_position);
    ret.calculateOpponentOpenAngles<op>(opKingSqr, m_position);
    return ret; 
}
