#include <position/hash_zobrist.hpp>
#include <array>
#include <core/chessboard.hpp>

namespace zobrist {
namespace internals {
std::array<std::array<u64, 12>, 64> table;
std::array<u64, 8> enpassant;
std::array<u64, 4> castling;
u64 black_to_move;
bool _initialized;

// splitmix64 with a fixed seed. The keys must be the same on every platform, rand() is implementation defined and
// gave Windows and Linux builds different keys, so different searches and benches.
u64 rand64(void) {
    static u64 state = 0x9E3779B97F4A7C15ull;
    u64 z = (state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

void initialize() {
    if (_initialized) return;

    black_to_move = rand64();

    for (u8 i = 0; i < 8; ++i)
        enpassant[i] = rand64();

    for (u8 i = 0; i < 4; ++i)
        castling[i] = rand64();

    for (u8 i = 0; i < 64; ++i) {
        for (u8 p = 0; p < 12; ++p) {
            table[i][p] = rand64();
        }
    }

    _initialized = true;
}

bool initialized() {
    return _initialized;
}

} // namespace internals

u64 computeBoardHash(const Chessboard& board) {
    u64 hash = 0;
    PositionReader reader = board.readPosition();
    auto itr = reader.begin();

    while (itr != reader.end()) {
        ChessPiece piece = itr.get();
        if (piece.isValid()) {
            hash = updatePieceHash(hash, piece, itr.square());
        }

        itr++;
    }

    byte castlingState = reader.castling().read();
    hash = updateCastlingHash(hash, castlingState);


    if (reader.enPassant()) {
        hash = updateEnPassantHash(hash, reader.enPassant().readSquare());
    }

    if (board.readToPlay() == Set::BLACK)
        hash ^= internals::black_to_move;

    return hash;
}

u64 updatePieceHash(const u64& oldHash, ChessPiece piece, Square position) {
    u8 pieceIndx = piece.index() + (piece.set() * 6);
    return oldHash ^ internals::table[*position][pieceIndx];
}

u64 updateEnPassantHash(const u64& oldHash, Square position) {
    return oldHash ^ internals::enpassant[to_file(position)];
}

u64 updateCastlingHash(const u64& oldHash, const u8 castlingState) {
    u64 hash = oldHash;
    if ((castlingState & 1) == 1)
        hash ^= internals::castling[0];
    if ((castlingState & 2) == 2)
        hash ^= internals::castling[1];
    if ((castlingState & 4) == 4)
        hash ^= internals::castling[2];
    if ((castlingState & 8) == 8)
        hash ^= internals::castling[3];

    return hash;
}

u64 updateBlackToMoveHash(const u64& oldHash) {
    return oldHash ^ internals::black_to_move;
}

} // namespace zobrist