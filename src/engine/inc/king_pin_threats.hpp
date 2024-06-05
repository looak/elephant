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
    [[nodiscard]] Bitboard pinned(Bitboard mask) const;

    [[nodiscard]] const Bitboard readEnPassantMask() const { return m_specialEnPassantMask; }
    [[nodiscard]] const Bitboard* readOpponentOpenAngles() const { return &m_opponentOpenAngles[0]; }
    // [[nodiscard]] const Bitboard& readKnightsAndPawns() const { return m_knightsAndPawns; }
    // [[nodiscard]] const Bitboard* readThreatenedAngles() const { return &m_threatenedAngles[0]; }

    template<Set us>
    void evaluate(Square kingSquare, const Position& position);

    template<Set op>
    void calculateOpponentOpenAngles(const Square kingSquare, const Position& position);

private:
    template<Set us>
    void calculateEnPassantPinThreat(Square kingSquare, const Position& position);

    Bitboard m_threatenedAngles[8];
    Bitboard m_specialEnPassantMask;
    Bitboard m_opponentOpenAngles[2];
    Bitboard m_knightsAndPawns;
    bool m_checkedAngles[8];
    bool m_knightOrPawnCheck;
};

#endif  // KING_PIN_THREATS_HEADER