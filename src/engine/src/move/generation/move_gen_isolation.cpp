#include <move/generation/move_gen_isolation.hpp>
#include <bitboard/attacks/attacks.hpp>
#include <position/position.hpp>

template <u8 pieceId> struct PieceIsoImpl;

// Pawn
template <>
struct PieceIsoImpl<pawnId> {
    template<Set us>
    static MovesMask apply(PositionReader position, Bitboard movesbb, Square source, const KingPinThreats<us>& pinThreats) {
        const size_t usIndx = static_cast<size_t>(us);

        Bitboard opMatCombined = position.material().combine<opposing_set<us>()>() | position.enPassant().readBitboard();
        Bitboard srcMask = Bitboard(squareMaskTable[*source]);

        const Bitboard pinned = pinThreats.pinned(srcMask);

        // special case for when there is a enpassant available.
        if (position.enPassant()) {        
            Bitboard potentialPin(pinThreats.readEnPassantMask() & srcMask);
            if (potentialPin) {
                opMatCombined ^= position.enPassant().readBitboard();
            }
        }

        Bitboard threatns;
        if ((srcMask & board_constants::boundsRelativeMasks[usIndx][west]).empty())
            threatns |= srcMask.shiftNorthWestRelative<us>();
        if ((srcMask & board_constants::boundsRelativeMasks[usIndx][east]).empty())
            threatns |= srcMask.shiftNorthEastRelative<us>();

        Bitboard isolatedbb = srcMask.shiftNorthRelative<us>();
        Bitboard unoccupied = ~(position.material().combine<us>() | opMatCombined);
        Bitboard doublePush = isolatedbb & pawn_constants::baseRank[usIndx] & unoccupied;
        isolatedbb |= doublePush.shiftNorthRelative<us>();
        isolatedbb &= unoccupied;    

        if (pinThreats.isChecked()) {
            Bitboard checksMask(pinThreats.checks());
            auto otherMask = squareMaskTable[(u32)position.enPassant().readTarget()];
            if (checksMask & otherMask) {
                checksMask |= position.enPassant().readBitboard();
            }
            isolatedbb &= checksMask;
        }

        if (srcMask & pinned) {
            isolatedbb &= pinned;
            threatns &= pinned;
        }
        return { movesbb & isolatedbb, movesbb & opMatCombined & threatns };
    }
};

// knight
template<>
struct PieceIsoImpl<knightId> {
    template<Set us>
    static MovesMask apply(PositionReader position, Bitboard movesbb, Square source, const KingPinThreats<us>& pinThreats) {
        u64 srcMask = squareMaskTable[*source];
        Bitboard opMatCombined = position.material().combine<opposing_set<us>()>();

        // figure out if piece is pinned
        const Bitboard pinned = pinThreats.pinned(srcMask);
        if (pinned.empty() == false) {
            movesbb &= pinned;
        }

        if (pinThreats.isChecked())
            movesbb &= pinThreats.checks();

        movesbb &= attacks::getKnightAttacks(*source);
        movesbb &= ~position.material().combine<us>();

        return { movesbb & ~opMatCombined, movesbb & opMatCombined };
    }
};

// bishop
template<>
struct PieceIsoImpl<bishopId> {
    template<Set us>
    static MovesMask apply(PositionReader position, Bitboard movesbb, Square source, const KingPinThreats<us>& pinThreats) {
    const Bitboard opMatCombined = position.material().combine<opposing_set<us>()>();
    const Bitboard allMaterial = position.material().combine();
    const Bitboard usMaterial = opMatCombined ^ allMaterial;

    // figure out if piece is pinned
    u64 srcMask = squareMaskTable[*source];
    const Bitboard pinned = pinThreats.pinned(srcMask);
    if (pinned.empty() == false) {
        movesbb &= pinned;
    }

    if (pinThreats.isChecked()) {
        movesbb &= pinThreats.checks();
    }

    movesbb &= attacks::getBishopAttacks(*source, allMaterial.read());
    movesbb &= ~usMaterial;

    return { movesbb & ~opMatCombined, movesbb & opMatCombined };
    }
};

// rook
template<>
struct PieceIsoImpl<rookId> {
    template<Set us>
    static MovesMask apply(PositionReader position, Bitboard movesbb, Square source, const KingPinThreats<us>& pinThreats) {
        const Bitboard opMatCombined = position.material().combine<opposing_set<us>()>();
        const Bitboard allMaterial = position.material().combine();
        const Bitboard usMaterial = opMatCombined ^ allMaterial;

        // figure out if piece is pinned
        u64 srcMask = squareMaskTable[*source];
        const Bitboard pinned = pinThreats.pinned(srcMask);
        if (pinned.empty() == false) {
            movesbb &= pinned;
        }

        if (pinThreats.isChecked()) {
            movesbb &= pinThreats.checks();
        }

        movesbb &= attacks::getRookAttacks(*source, allMaterial.read());
        movesbb &= ~usMaterial;

        return { movesbb & ~opMatCombined, movesbb & opMatCombined };
    }
};

// queen
template<>
struct PieceIsoImpl<queenId> {
    template<Set us>
    static MovesMask apply(PositionReader position, Bitboard movesbb, Square source, const KingPinThreats<us>& pinThreats) {
        auto ortho = PieceIsoImpl<rookId>::apply<us>(position, movesbb, source, pinThreats);
        auto diag = PieceIsoImpl<bishopId>::apply<us>(position, movesbb, source, pinThreats);
        return { ortho.quiets | diag.quiets, ortho.captures | diag.captures };
    }
};

template<Set us, u8 pieceId>
MovesMask PieceIsolator<us, pieceId>::isolate(Square src) const {
    return PieceIsoImpl<pieceId>::apply<us>(m_position, m_movesMask, src, m_pinThreats);
}

template MovesMask PieceIsolator<Set::WHITE, pawnId>::isolate(Square) const;
template MovesMask PieceIsolator<Set::BLACK, pawnId>::isolate(Square) const;
template MovesMask PieceIsolator<Set::WHITE, knightId>::isolate(Square) const;
template MovesMask PieceIsolator<Set::BLACK, knightId>::isolate(Square) const;
template MovesMask PieceIsolator<Set::WHITE, bishopId>::isolate(Square) const;
template MovesMask PieceIsolator<Set::BLACK, bishopId>::isolate(Square) const;
template MovesMask PieceIsolator<Set::WHITE, rookId>::isolate(Square) const;
template MovesMask PieceIsolator<Set::BLACK, rookId>::isolate(Square) const;
template MovesMask PieceIsolator<Set::WHITE, queenId>::isolate(Square) const;
template MovesMask PieceIsolator<Set::BLACK, queenId>::isolate(Square) const;