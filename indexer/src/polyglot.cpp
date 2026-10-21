#include "polyglot.hpp"
#include "polyglot_keys.hpp"

#include <format>

#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <core/chessboard.hpp>

namespace indexer {
namespace {
    constexpr size_t c_castlingKeys = 768;
    constexpr size_t c_enPassantKeys = 772;
    constexpr size_t c_whiteToMoveKey = 780;

    constexpr u64 bit(i32 square) { return u64(1) << square; }
} // namespace

u64 polyglotHash(const Chessboard& board)
{
    const PositionReader position = board.readPosition();
    const bool whiteToMove = board.readToPlay() == Set::WHITE;
    u64 hash = 0;

    // a piece's key: 64 squares (a1 first) for each kind, black pawn, white pawn, black knight ... white king.
    for (Set set : { Set::WHITE, Set::BLACK }) {
        const size_t colour = set == Set::WHITE ? 1 : 0;
        for (u8 pieceId = pawnId; pieceId <= kingId; ++pieceId) {
            u64 pieces = position.material().read(set, pieceId).read();
            while (pieces) {
                const size_t square = static_cast<size_t>(intrinsics::lsbIndex(pieces));
                hash ^= c_polyglotKeys[64 * (2 * size_t(pieceId) + colour) + square];
                pieces &= pieces - 1;
            }
        }
    }

    // a right counts only with the king and that rook on their squares, as python-chess cleans castling rights.
    const u64 whiteKing = position.material().read(Set::WHITE, kingId).read();
    const u64 blackKing = position.material().read(Set::BLACK, kingId).read();
    const u64 whiteRooks = position.material().read(Set::WHITE, rookId).read();
    const u64 blackRooks = position.material().read(Set::BLACK, rookId).read();
    const auto& castling = position.castling();
    const bool whiteHome = whiteKing & bit(4);   // e1
    const bool blackHome = blackKing & bit(60);  // e8
    if (castling.hasWhiteKingSide() && whiteHome && (whiteRooks & bit(7)))
        hash ^= c_polyglotKeys[c_castlingKeys];
    if (castling.hasWhiteQueenSide() && whiteHome && (whiteRooks & bit(0)))
        hash ^= c_polyglotKeys[c_castlingKeys + 1];
    if (castling.hasBlackKingSide() && blackHome && (blackRooks & bit(63)))
        hash ^= c_polyglotKeys[c_castlingKeys + 2];
    if (castling.hasBlackQueenSide() && blackHome && (blackRooks & bit(56)))
        hash ^= c_polyglotKeys[c_castlingKeys + 3];

    const Square epSquare = position.enPassant().readSquare();
    if (epSquare != Square::NullSQ) {
        const i32 ep = static_cast<i32>(epSquare);
        const u64 pushed = bit(whiteToMove ? ep - 8 : ep + 8);
        const u64 beside = ((pushed << 1) & ~board_constants::fileaMask) | ((pushed >> 1) & ~board_constants::filehMask);
        const u64 capturers = position.material().read(whiteToMove ? Set::WHITE : Set::BLACK, pawnId).read();
        if (beside & capturers)
            hash ^= c_polyglotKeys[c_enPassantKeys + static_cast<size_t>(ep % 8)];
    }

    if (whiteToMove)
        hash ^= c_polyglotKeys[c_whiteToMoveKey];
    return hash;
}

std::string toHex(u64 hash) { return std::format("{:016x}", hash); }

} // namespace indexer
