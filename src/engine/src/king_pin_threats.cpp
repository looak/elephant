#include "king_pin_threats.hpp"
#include "position.hpp"

KingPinThreats::KingPinThreats() :
    m_knightsAndPawns(0),
    m_knightOrPawnCheck(false)
{
    for (u8 i = 0; i < 8; ++i) {
        m_threatenedAngles[i] = 0;
        m_checkedAngles[i] = false;
    }
}

bool
KingPinThreats::isChecked() const
{
    for (auto checked : m_checkedAngles) {
        if (checked)
            return true;
    }
    return m_knightOrPawnCheck;
}

u32
KingPinThreats::isCheckedCount() const
{
    u32 count = 0;
    for (auto checked : m_checkedAngles) {
        if (checked) {
            ++count;
        }
    }
    if (m_knightOrPawnCheck) {
        ++count;
    }
    return count;
}

Bitboard
KingPinThreats::combined() const
{
    Bitboard combined = 0;
    for (auto threatened : m_threatenedAngles) {
        combined |= threatened;
    }

    combined |= m_knightsAndPawns;
    return combined;
}

Bitboard
KingPinThreats::pins() const
{
    Bitboard combined = 0;
    for (u8 i = 0; i < 8; ++i) {
        if (m_checkedAngles[i] == true)
            continue;
        combined |= m_threatenedAngles[i];
    }
    return combined;
}

Bitboard
KingPinThreats::checks() const
{
    Bitboard combined = 0;
    for (u8 i = 0; i < 8; ++i) {
        if (m_checkedAngles[i] == false)
            continue;
        combined |= m_threatenedAngles[i];
    }
    if (m_knightOrPawnCheck) {
        combined |= m_knightsAndPawns;
    }
    return combined;
}

void
KingPinThreats::evaluate(Set set, Notation kingSquare, const Position& position,
                         const SlidingMaterialMasks& opponentSlidingMask)
{
    byte moveCount = ChessPieceDef::MoveCount(kingId);

    const Set opSet = ChessPiece::FlipSet(set);
    const Bitboard diagonalMaterial = position.readMaterial(opSet).bishops() | position.readMaterial(opSet).queens();
    const Bitboard orthogonalMaterial = position.readMaterial(opSet).rooks() | position.readMaterial(opSet).queens();
    const Bitboard allMaterial = position.readMaterial<Set::WHITE>().combine() | position.readMaterial<Set::BLACK>().combine();

    const Bitboard slideMatCache[2]{opponentSlidingMask.orthogonal & orthogonalMaterial,
                                    opponentSlidingMask.diagonal & diagonalMaterial};

    u8 matCount = 0;
    bool threatened = (!diagonalMaterial.empty() || !orthogonalMaterial.empty());
    const bool threatenedReset = threatened;
    bool openAngle = true;

    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
        matCount = 0;
        threatened = threatenedReset;
        openAngle = true;
        byte curSqr = kingSquare.index();
        signed short dir = ChessPieceDef::Moves0x88(kingId, moveIndx);

        m_threatenedAngles[moveIndx] = ~universe;

        bool diagonal = ChessPieceDef::IsDiagonalMove(dir);
        Bitboard slideMat = slideMatCache[diagonal];
        Bitboard* openAnglePtr = &m_openAngles[diagonal];

        Bitboard mvMask = 0;
        do {
            // build a 0x88 square out of current square.
            signed char sq0x88 = to0x88(curSqr);
            // do move
            sq0x88 += dir;
            if (sq0x88 & 0x88)  // validate move, are we still on the board?
                break;

            // convert our sqr0x88 back to a square index
            curSqr = fr0x88(sq0x88);
            // build a square mask from current square
            u64 sqrMask = squareMaskTable[curSqr];

            if (allMaterial & sqrMask) {
                openAngle = false;
                matCount++;
            }

            if (slideMat & sqrMask) {
                threatened = false;
            }

            if (openAngle)
                *openAnglePtr |= sqrMask;

            mvMask |= sqrMask;

        } while (threatened || openAngle);

        // comparing against two here since we'll find the sliding piece causing the pin
        // and at least one piece in between our king and this piece. This found piece isn't
        // necessarily pinned, unless there are no more pieces between king and sliding piece,
        // in that case they are pinned.
        if (mvMask & slideMat && matCount <= 2) {
            m_threatenedAngles[moveIndx] |= mvMask;
            if (matCount == 1) {
                m_checkedAngles[moveIndx] = true;
            }
        }
    }

    const u64 knightMat = position.readMaterial(opSet).knights().read();

    m_knightsAndPawns = ~universe;
    if (knightMat > 0) {
        // figure out if we're checked by a knight
        moveCount = ChessPieceDef::MoveCount(knightId);

        for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
            byte curSqr = kingSquare.index();
            signed short dir = ChessPieceDef::Moves0x88(knightId, moveIndx);

            u64 mvMask = 0;

            // build a 0x88 square out of current square.
            signed char sq0x88 = to0x88(curSqr);
            // do move
            sq0x88 += dir;
            if (sq0x88 & 0x88)  // validate move, are we still on the board?
            {
                continue;
            }

            // convert our sqr0x88 back to a square index
            curSqr = fr0x88(sq0x88);
            // build a square mask from current square
            u64 sqrMask = squareMaskTable[curSqr];
            mvMask |= sqrMask;

            if (mvMask & knightMat) {
                m_knightsAndPawns |= mvMask;
                m_knightOrPawnCheck = true;
            }
        }
    }

    if (position.readMaterial(opSet).pawns().empty() == false) {
        // figure out if we're checked by a pawn
        i8 pawnMod = set == Set::WHITE ? 1 : -1;
        auto pawnSqr = Notation(kingSquare.file + 1, kingSquare.rank + pawnMod);
        if (Position::IsValidSquare(pawnSqr)) {
            u64 sqrMak = squareMaskTable[pawnSqr.index()];
            sqrMak &= position.readMaterial(opSet).pawns().read();
            if (sqrMak > 0) {
                m_knightsAndPawns |= sqrMak;
                m_knightOrPawnCheck = true;
            }
        }
        pawnSqr = Notation(kingSquare.file - 1, kingSquare.rank + pawnMod);
        if (Position::IsValidSquare(pawnSqr)) {
            u64 sqrMak = squareMaskTable[pawnSqr.index()];
            sqrMak &= position.readMaterial(opSet).pawns().read();
            if (sqrMak > 0) {
                m_knightsAndPawns |= sqrMak;
                m_knightOrPawnCheck = true;
            }
        }
    }
}