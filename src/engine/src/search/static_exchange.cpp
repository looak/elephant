#include <search/static_exchange.hpp>

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/intrinsics.hpp>
#include <material/material_mask.hpp>

namespace see {
namespace {

// every piece of either set attacking sqr, sliders seen through occupancy.
u64 attackersTo(const MaterialPositionMask& material, u32 sqr, u64 occupancy) {
    const u64 sqrMask = 1ull << sqr;
    const u64 queens = material.queens().read();
    const u64 diagonals = material.bishops().read() | queens;
    const u64 orthogonals = material.rooks().read() | queens;

    // a white pawn attacks sqr from the squares a black pawn on sqr would attack, and vice versa.
    return (attacks::pawnAttacks<Set::BLACK>(sqrMask) & material.whitePawns().read())
         | (attacks::pawnAttacks<Set::WHITE>(sqrMask) & material.blackPawns().read())
         | (attacks::getKnightAttacks(sqr) & material.knights().read())
         | (attacks::getKingAttacks(sqr) & material.kings().read())
         | (attacks::getBishopAttacks(sqr, occupancy) & diagonals)
         | (attacks::getRookAttacks(sqr, occupancy) & orthogonals);
}

} // namespace

bool ge(const MaterialPositionMask& material, PackedMove move, i32 threshold) {
    if (move.isCastling())
        return 0 >= threshold;

    const u32 from = static_cast<u32>(move.source());
    const u32 to = static_cast<u32>(move.target());
    const ChessPiece mover = material.pieceAt(move.sourceSqr());
    const u8 us = toSetId(mover.getSet());

    // what the move itself wins, and the piece then standing on the target square.
    i32 gain = 0;
    u8 onTarget = mover.index();
    if (move.isEnPassant())
        gain = value[pawnId];
    else if (move.isCapture())
        gain = value[material.pieceAt(move.targetSqr()).index()];
    if (move.isPromotion()) {
        onTarget = static_cast<u8>(move.readPromoteToPieceType() - 1);
        gain += value[onTarget] - value[pawnId];
    }

    // even if nothing recaptures it doesn't reach the threshold.
    i32 balance = gain - threshold;
    if (balance < 0)
        return false;

    // even losing the piece on the target square for nothing still reaches it.
    balance -= value[onTarget];
    if (balance >= 0)
        return true;

    u64 occupancy = (material.combine().read() ^ (1ull << from)) | (1ull << to);
    if (move.isEnPassant()) {
        // the captured pawn stands behind the target square, as seen from the capturing side.
        const u32 captured = us == toSetId(Set::WHITE) ? to - 8 : to + 8;
        occupancy ^= 1ull << captured;
    }

    const u64 queens = material.queens().read();
    const u64 diagonals = material.bishops().read() | queens;
    const u64 orthogonals = material.rooks().read() | queens;
    u64 attackers = attackersTo(material, to, occupancy) & occupancy;

    // side is the one to recapture next. balance is from its point of view when the loop decides, the side that
    // can't or won't recapture any more loses the exchange.
    u8 side = opposing_set(us);
    while (true) {
        const u64 ours = attackers & material.set(side).read();
        if (ours == 0)
            break;

        // least valuable attacker.
        u8 piece = pawnId;
        u64 pieceAttackers = 0;
        for (; piece <= kingId; ++piece) {
            pieceAttackers = ours & material.read(piece).read();
            if (pieceAttackers != 0)
                break;
        }

        // it leaves the square it attacks from, sliders behind it may now see through.
        occupancy ^= pieceAttackers & (~pieceAttackers + 1);
        if (piece == pawnId || piece == bishopId || piece == queenId)
            attackers |= attacks::getBishopAttacks(to, occupancy) & diagonals;
        if (piece == rookId || piece == queenId)
            attackers |= attacks::getRookAttacks(to, occupancy) & orthogonals;
        attackers &= occupancy;

        side = opposing_set(side);

        // negamax the balance, the recapturing piece is now the one en prise.
        balance = -balance - 1 - value[piece];
        if (balance >= 0) {
            // a king can't recapture into a square that's still defended, the other side wins it back.
            if (piece == kingId && (attackers & material.set(side).read()) != 0)
                side = opposing_set(side);
            break;
        }
    }

    // the side to move at the end lost the exchange.
    return side != us;
}

} // namespace see
