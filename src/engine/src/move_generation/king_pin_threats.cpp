#include <move_generation/king_pin_threats.hpp>
#include "attacks/attacks.hpp"
#include <position/position.hpp>
#include "rays/rays.hpp"

KingPinThreats::KingPinThreats() :
    m_specialEnPassantMask(0),
    m_knightsAndPawns(0),
    m_knightOrPawnCheck(false)
{
    for (u8 i = 0; i < 8; ++i) {
        m_threatenedAngles[i] = 0;
        m_checkedAngles[i] = false;
    }

    m_opponentOpenAngles[0] = 0;
    m_opponentOpenAngles[1] = 0;
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
KingPinThreats::pinned(Bitboard mask) const
{
    for (u8 i = 0; i < 8; ++i) {
        if ((mask & m_threatenedAngles[i]).empty() == false)
            return m_threatenedAngles[i];
    }
    return Bitboard(0);
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

template<Set us>
void KingPinThreats::calculateEnPassantPinThreat(Square kingSquare, PositionReader position)
{
    if (position.enPassant() == false) {
        return;
    }

    const auto material = position.material();

    constexpr Set op = opposing_set<us>();
    constexpr size_t opIndx = static_cast<size_t>(op);
    Bitboard kingSquareMask = squareMaskTable[*kingSquare];

    if (kingSquareMask & board_constants::enPassantRankRelative[opIndx]) {
        const Bitboard usMaterial = material.combine<us>();
        const Bitboard opMaterial = material.combine<op>();
        const Bitboard allMaterial = usMaterial | opMaterial;
        const Bitboard orthogonalMaterial = material.rooks<op>() | material.queens<op>();

        Bitboard riskOfPin = allMaterial & board_constants::enPassantRankRelative[opIndx];

        if ((riskOfPin & orthogonalMaterial).empty())
            return;  // no one to pin us on this rank.

        const Bitboard usPawns = material.pawns<us>();

        if ((usPawns & board_constants::enPassantRankRelative[opIndx]).empty())
            return;  // no pawns on this rank to pin.

        Square epTargetSquare = position.enPassant().readTarget();
        Notation epTarget(epTargetSquare);

        Bitboard resultMask;
        Notation kingNotation(kingSquare);
        if (epTarget.file > kingNotation.file) {
            do {
                kingSquareMask = kingSquareMask.shiftEast();
                resultMask |= kingSquareMask;

                if (kingSquareMask & orthogonalMaterial)
                    break;

            } while ((kingSquareMask & board_constants::filehMask).empty());
        }
        else {
            do {
                kingSquareMask = kingSquareMask.shiftWest();
                resultMask |= kingSquareMask;

                if (kingSquareMask & orthogonalMaterial)
                    break;

            } while ((kingSquareMask & board_constants::fileaMask).empty());
        }
        auto mat = allMaterial ^ orthogonalMaterial;

        if ((resultMask & mat).count() > 2)
            return;  // we're not pinned, there are more than two pieces between us and the sliding piece.

        m_specialEnPassantMask = resultMask;
    }
}

template void KingPinThreats::calculateEnPassantPinThreat<Set::WHITE>(Square, PositionReader);
template void KingPinThreats::calculateEnPassantPinThreat<Set::BLACK>(Square, PositionReader);

template<Set us>
void KingPinThreats::evaluate(Square kingSquare, PositionReader position)
{
    constexpr Set op = opposing_set<us>();
    const Bitboard diagonalMaterial = position.material().bishops<op>() | position.material().queens<op>();
    const Bitboard orthogonalMaterial = position.material().rooks<op>() | position.material().queens<op>();
    const Bitboard usMaterial = position.material().combine<us>();
    const Bitboard opMaterial = position.material().combine<op>();

    // clear threatened angles
    for (u8 i = 0; i < 8; ++i) {
        m_threatenedAngles[i] = 0;
        m_checkedAngles[i] = false;
    }
    u8 threatIndex = 0;

    Bitboard kingRays = attacks::getRookAttacks(*kingSquare, opMaterial.read());
    Bitboard potentialPinsAndCheckers = kingRays & orthogonalMaterial;

    while (potentialPinsAndCheckers.empty() == false) {
        u32 potentialChecker = potentialPinsAndCheckers.popLsb();

        Bitboard ray = ray::getRay(*kingSquare, potentialChecker);
        if (ray.empty())
            continue;

        int pinCount = (ray & usMaterial).count();


        if (pinCount == 0) {
            m_threatenedAngles[threatIndex] = ray;
            m_checkedAngles[threatIndex] = true;
            threatIndex++;
        }
        else if (pinCount == 1) {
            m_threatenedAngles[threatIndex] = ray;
            threatIndex++;
        }
    }

    kingRays = attacks::getBishopAttacks(*kingSquare, opMaterial.read());
    potentialPinsAndCheckers = kingRays & diagonalMaterial;
    while (potentialPinsAndCheckers.empty() == false) {
        u32 potentialChecker = potentialPinsAndCheckers.popLsb();

        Bitboard ray = ray::getRay(*kingSquare, potentialChecker);
        if (ray.empty())
            continue;

        int pinCount = (ray & usMaterial).count();

        if (pinCount == 0) {
            m_threatenedAngles[threatIndex] = ray;
            m_checkedAngles[threatIndex] = true;
            threatIndex++;
        }
        else if (pinCount == 1) {
            m_threatenedAngles[threatIndex] = ray;
            threatIndex++;
        }
    }

    const u64 knightMat = position.material().knights<op>().read();

    m_knightsAndPawns = ~universe;
    if (knightMat > 0) {
        // figure out if we're checked by a knight
        Bitboard knightAttacks = attacks::getKnightAttacks(*kingSquare);
        Bitboard knights = position.material().knights<op>();
        Bitboard mvMask = knightAttacks & knights;

        if (mvMask.empty() == false) {
            m_knightsAndPawns = mvMask;
            m_knightOrPawnCheck = true;
        }
    }

    if (position.material().pawns<op>().empty() == false) {
        Bitboard kingMask = position.material().king<us>();
        Bitboard pawnsMask = position.material().pawns<op>();
        u8 usIndx = static_cast<u8>(us);
        // special case for a file & h file, removing pawns from a & h file respectively
        // so we don't shift them "off" the board and we shift them only in one direction.
        // cache them and then we combine it with the main piecebb at the end.
        Bitboard piecebb = kingMask;
        Bitboard westFile = kingMask & board_constants::boundsRelativeMasks[usIndx][west];
        piecebb &= ~westFile;
        westFile = westFile.shiftNorthEastRelative<us>();

        Bitboard eastFile = kingMask & board_constants::boundsRelativeMasks[usIndx][east];
        piecebb &= ~eastFile;
        eastFile = eastFile.shiftNorthWestRelative<us>();

        Bitboard threatbb = westFile | eastFile;
        threatbb |= piecebb.shiftNorthWestRelative<us>();
        threatbb |= piecebb.shiftNorthEastRelative<us>();

        if (threatbb & pawnsMask) {
            m_knightsAndPawns |= threatbb & pawnsMask;
            m_knightOrPawnCheck = true;
        }
    }

    calculateEnPassantPinThreat<us>(kingSquare, position);
}

template void KingPinThreats::evaluate<Set::WHITE>(Square, PositionReader);
template void KingPinThreats::evaluate<Set::BLACK>(Square, PositionReader);

template<Set op>
void KingPinThreats::calculateOpponentOpenAngles(const Square kingSquare, PositionReader position)
{
    const Bitboard opMaterial = position.material().combine<op>();
    constexpr Set us = opposing_set<op>();
    const Bitboard usMaterial = position.material().combine<us>();
    const Bitboard allMaterial = usMaterial | opMaterial;

    u64 diagonals = attacks::getBishopAttacks(*kingSquare, allMaterial.read());
    diagonals &= ~usMaterial.read();
    m_opponentOpenAngles[1] = diagonals;

    u64 orthogonals = attacks::getRookAttacks(*kingSquare, allMaterial.read());
    orthogonals &= ~usMaterial.read();
    m_opponentOpenAngles[0] = orthogonals;
}

template void KingPinThreats::calculateOpponentOpenAngles<Set::WHITE>(Square, PositionReader);
template void KingPinThreats::calculateOpponentOpenAngles<Set::BLACK>(Square, PositionReader);