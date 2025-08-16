#pragma once

#include "bitboard.hpp"
#include <material/chess_piece_defines.hpp>
#include <position/position_accessors.hpp>

struct SlidingMaterialMasks {
    Bitboard orthogonal;
    Bitboard diagonal;
};

template<Set us>
class KingPinThreats {
public:
    KingPinThreats(Square kingSquare, PositionReader position);

    [[nodiscard]] u32 isCheckedCount() const;
    [[nodiscard]] bool isChecked() const;
    [[nodiscard]] Bitboard combined() const;
    [[nodiscard]] Bitboard pins() const;
    [[nodiscard]] Bitboard checks() const;
    [[nodiscard]] Bitboard pinned(Bitboard mask) const;

    [[nodiscard]] const Bitboard readEnPassantMask() const { return m_specialEnPassantMask; }
    [[nodiscard]] const Bitboard* readOpponentOpenAngles() const { return &m_opponentOpenAngles[0]; }
    // [[nodiscard]] const Bitboard& readKnightsAndPawns() const { return m_knightsAndPawns; }
    // [[nodiscard]] const Bitboard* readThreatenedAngles() const { return &m_threatenedAngles[0]; }
    
    void calculateOpponentOpenAngles(const Square kingSquare, PositionReader position);
    
private:
    void compute(Square kingSquare, PositionReader position);
    void calculateEnPassantPinThreat(Square kingSquare, PositionReader position);

    Bitboard m_threatenedAngles[8];
    Bitboard m_specialEnPassantMask;
    Bitboard m_opponentOpenAngles[2];
    Bitboard m_knightsAndPawns;
    bool m_checkedAngles[8];
    bool m_knightOrPawnCheck;
};