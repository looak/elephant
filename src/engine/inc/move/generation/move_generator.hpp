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

#include <move/generation/king_pin_threats.hpp>
#include <move/generation/move_bulk_generator.hpp>
#include <move/generation/move_gen_isolation.hpp>
#include <move/generation/move_ordering_view.hpp>
#include <position/position_accessors.hpp>

struct MoveGenParams {    
    const MoveOrderingView* ordering = nullptr;
    MoveTypes moveFilter = MoveTypes::ALL;
    bool inCheck = false;
    u8 pieceIdFlag = 0; // bitmask for which piece types to generate moves for

    void setPawns(bool value) {
        if (value) pieceIdFlag |= 0b00000001;
        else pieceIdFlag &= 0b11111110;
    };
    void setKnights(bool value) {
        if (value) pieceIdFlag |= 0b00000010;
        else pieceIdFlag &= 0b11111101;
    };
    void setBishops(bool value) {
        if (value) pieceIdFlag |= 0b00000100;
        else pieceIdFlag &= 0b11111011;
    };
    void setRooks(bool value) {
        if (value) pieceIdFlag |= 0b00001000;
        else pieceIdFlag &= 0b11110111;
    };
    void setQueens(bool value) {
        if (value) pieceIdFlag |= 0b00010000;
        else pieceIdFlag &= 0b11101111;
    };
    void setKings(bool value) {
        if (value) pieceIdFlag |= 0b00100000;
        else pieceIdFlag &= 0b11011111;
    };
    void setAll(bool value) {
        setPawns(value);
        setKnights(value);
        setBishops(value);
        setRooks(value);
        setQueens(value);
        setKings(value);
    };

    bool hasPawns() const { return (pieceIdFlag & 0b00000001) != 0; };
    bool hasKnights() const { return (pieceIdFlag & 0b00000010) != 0; };
    bool hasBishops() const { return (pieceIdFlag & 0b00000100) != 0; };
    bool hasRooks() const { return (pieceIdFlag & 0b00001000) != 0; };
    bool hasQueens() const { return (pieceIdFlag & 0b00010000) != 0; };
    bool hasKings() const { return (pieceIdFlag & 0b00100000) != 0; };
};

namespace move_generator_constants {
// priority values for move generator
// higher value means higher priority
constexpr u16 capturePriority = 1000;
constexpr u16 promotionPriority = 2000;
constexpr u16 checkPriority = 900;
constexpr u16 ttMovePriority = 4000;
constexpr u16 pvMovePriority = 5000;
constexpr u16 killerMovePriority = 800;
} // namespace move_generator_constants

template<Set _us>
class MoveGenerator {
public:
    explicit MoveGenerator(PositionReader position, MoveGenParams& params);

    [[nodiscard]] PackedMove pop();
    PackedMove peek();

    bool isChecked() const {
        return m_pinThreats.isChecked();
    }

#ifdef DEVELOPMENT_BUILD
    [[nodiscard]] std::vector<PrioritizedMove> moves();
#endif

private:
enum class Stage{
    PV_MOVE,
    CAPTURES,
    KILLERS,
    QUIETS,
    DONE
};

/*
Consider this flow:
    enum class Stage {
        PV_MOVE,
        CAPTURES_GEN,
        CAPTURES_SORT,
        KILLERS,
        QUIETS_GEN,
        QUIETS_SORT,
        DONE
    };
*/
    KingPinThreats<_us> computeKingPinThreats();
    PrioritizedMove internalGenerateMoves();
    void internalGenerateMovesOrdered();
    void internalGeneratePawnMoves(BulkMoveGenerator bulkMoveGen);
    void internalBuildPawnPromotionMoves(PackedMove move, i32 dstSqr);
    template<u8 pieceId>
    void internalGenerateMovesGeneric(BulkMoveGenerator bulkMoveGen);
    void internalGenerateKingMoves(BulkMoveGenerator bulkMoveGen);

    Bitboard internalCallBulkGeneratorForPiece(u8 pieceId, BulkMoveGenerator bulkMoveGen);

    void buildPackedMoveFromBitboard(u8 pieceId, Bitboard movesbb, Square srcSqr, bool capture);

    void sortMoves();

    std::array<PrioritizedMove, 256> m_movesBuffer; // 1kb
    
    KingPinThreats<_us> m_pinThreats;
    PositionProxy<PositionReadOnlyPolicy> m_position;
    
    u32 m_currentMoveIndx;
    u32 m_moveCount;
    bool m_movesGenerated;
    Stage m_stage;
    MoveGenParams& m_params;
};


template<Set us>
KingPinThreats<us> MoveGenerator<us>::computeKingPinThreats() {
    constexpr Set op = opposing_set<us>();  
    Square kingSqr = static_cast<Square>(m_position.material().king<us>().lsbIndex());
    Square opKingSqr = static_cast<Square>(m_position.material().king<op>().lsbIndex());

    KingPinThreats<us> ret;
    //ret.evaluate(kingSqr, m_position);
    ret.calculateOpponentOpenAngles(opKingSqr, m_position);
    return ret; 
}

template<Set us> 
template<u8 pieceId>
void MoveGenerator<us>::internalGenerateMovesGeneric(BulkMoveGenerator bulkMoveGen)
{
    const Bitboard movesbb = internalCallBulkGeneratorForPiece(pieceId, bulkMoveGen);
    if (movesbb.empty())
        return;

    Bitboard pieces = m_position.material().read<us>(pieceId);
    PieceIsolator<us, pieceId> isolator(m_position, movesbb, m_pinThreats);

    while (pieces.empty() == false) {
        // build source square and remove knight from cached material bitboard.
        const Square srcSqr = toSquare(pieces.popLsb());
        auto [isolatedMoves, isolatedCaptures] = isolator.isolate(srcSqr);
        buildPackedMoveFromBitboard(pieceId, isolatedCaptures, srcSqr, /*are captures*/ true);
        buildPackedMoveFromBitboard(pieceId, isolatedMoves, srcSqr, /*are captures*/ false);
    }
}


typedef MoveGenerator<Set::WHITE> WhiteMoveGen;
typedef MoveGenerator<Set::BLACK> BlackMoveGen;