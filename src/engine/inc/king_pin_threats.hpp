#ifndef KING_PIN_THREATS_HEADER
#define KING_PIN_THREATS_HEADER
#include "bitboard.hpp"
#include "chess_piece_defines.hpp"
#include "notation.h"

class Position;

struct SlidingMaterialMasks {
    Bitboard orthogonal;
    Bitboard diagonal;
};

struct KingPinThreats {
    KingPinThreats();

    [[nodiscard]] u32 isCheckedCount() const;
    [[nodiscard]] bool isChecked() const;
    [[nodiscard]] Bitboard combined() const;
    [[nodiscard]] Bitboard pins() const;
    [[nodiscard]] Bitboard checks() const;

    [[nodiscard]] const Bitboard* readOpenAngles() const { return &m_openAngles[0]; }
    // [[nodiscard]] const Bitboard& readKnightsAndPawns() const { return m_knightsAndPawns; }
    // [[nodiscard]] const Bitboard* readThreatenedAngles() const { return &m_threatenedAngles[0]; }

    void evaluate(Set set, Notation kingSquare, const Position& position, const SlidingMaterialMasks& opponentSlidingMask);

private:
    Bitboard m_threatenedAngles[8];
    Bitboard m_openAngles[2];
    Bitboard m_knightsAndPawns;
    bool m_checkedAngles[8];
    bool m_knightOrPawnCheck;
};

#endif  // KING_PIN_THREATS_HEADER