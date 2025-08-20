#include <position/hash_zobrist.hpp>
#include <array>
#include <core/chessboard.hpp>

// https://stackoverflow.com/questions/33010010/how-to-generate-random-64-bit-unsigned-integer-in-c
#define IMAX_BITS(m) ((m) / ((m) % 255 + 1) / 255 % 255 * 8 + 7 - 86 / ((m) % 255 + 12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)
//_Static_assert((RAND_MAX& (RAND_MAX + 1u)) == 0, "RAND_MAX not a Mersenne number");


namespace zobrist {
namespace internals {
std::array<std::array<u64, 12>, 64> table;
std::array<u64, 8> enpassant;
std::array<u64, 4> castling;
u64 black_to_move;
bool _initialized;

u64 rand64(void) {
    u64 r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
        r <<= RAND_MAX_WIDTH;
        r ^= (unsigned)rand();
    }
    return r;
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
    return 0;
}

u64 updatePieceHash(const u64& oldHash, ChessPiece piece, Square position) {
    u8 pieceIndx = piece.index() + (piece.set() * 6);
    return oldHash ^ internals::table[*position][pieceIndx];
}

u64 updateEnPassantHash(const u64& oldHash, Square position) {
    return oldHash ^ internals::enpassant[toFile(position)];
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

} // namespace zorbist


// u64
// ZorbistHash::HashBoard(const Chessboard& board) const
// {
//     u64 hash = 0;
    // auto itr = board.begin();

    // while (itr != board.end()) {
    //     ChessPiece piece = board.readPieceAt(itr.square());
    //     if (piece.isValid()) {
    //         u8 pieceIndx = piece.index() + (piece.set() * 6);
    //         hash ^= table[itr.index()][pieceIndx];
    //     }

    //     itr++;
    // }

    // byte castlingState = board.readCastlingState().read();

    // if ((castlingState & 1) == 1)
    //     hash ^= castling[0];
    // if ((castlingState & 2) == 2)
    //     hash ^= castling[1];
    // if ((castlingState & 4) == 4)
    //     hash ^= castling[2];
    // if ((castlingState & 8) == 8)
    //     hash ^= castling[3];

    // if (board.readPosition().readEnPassant()) {
    //     Notation ep(board.readPosition().readEnPassant().readSquare());
    //     hash ^= enpassant[ep.file];
    // }

    // if (board.readToPlay() == Set::BLACK)
    //     hash ^= black_to_move;

//     return hash;
// }