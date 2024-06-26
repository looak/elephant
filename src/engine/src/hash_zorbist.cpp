#include "hash_zorbist.h"
#include "chessboard.h"

// https://stackoverflow.com/questions/33010010/how-to-generate-random-64-bit-unsigned-integer-in-c
#define IMAX_BITS(m) ((m) / ((m) % 255 + 1) / 255 % 255 * 8 + 7 - 86 / ((m) % 255 + 12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)
//_Static_assert((RAND_MAX& (RAND_MAX + 1u)) == 0, "RAND_MAX not a Mersenne number");

u64
rand64(void)
{
    u64 r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
        r <<= RAND_MAX_WIDTH;
        r ^= (unsigned)rand();
    }
    return r;
}

ZorbistHash ZorbistHash::instance;

const ZorbistHash&
ZorbistHash::Instance()
{
    return instance;
}

void
ZorbistHash::GenerateZorbistTable()
{
    if (initialized)
        LOG_ERROR() << "Zorbist Hash Table has already been initialized!";

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

    initialized = true;
}

u64
ZorbistHash::HashBoard(const Chessboard& board) const
{
    u64 hash = 0;
    auto itr = board.begin();

    while (itr != board.end()) {
        ChessPiece piece = board.readPieceAt(itr.square());
        if (piece.isValid()) {
            u8 pieceIndx = piece.index() + (piece.set() * 6);
            hash ^= table[itr.index()][pieceIndx];
        }

        itr++;
    }

    byte castlingState = board.readCastlingState().read();

    if ((castlingState & 1) == 1)
        hash ^= castling[0];
    if ((castlingState & 2) == 2)
        hash ^= castling[1];
    if ((castlingState & 4) == 4)
        hash ^= castling[2];
    if ((castlingState & 8) == 8)
        hash ^= castling[3];

    if (board.readPosition().readEnPassant()) {
        Notation ep(board.readPosition().readEnPassant().readSquare());
        hash ^= enpassant[ep.file];
    }

    if (board.readToPlay() == Set::BLACK)
        hash ^= black_to_move;

    return hash;
}

u64
ZorbistHash::HashPiecePlacement(const u64& oldHash, ChessPiece piece, Notation position) const
{
    u8 pieceIndx = piece.index() + (piece.set() * 6);
    return oldHash ^ table[position.index()][pieceIndx];
}

u64
ZorbistHash::HashEnPassant(const u64& oldHash, Notation position) const
{
    return oldHash ^ enpassant[position.file];
}

u64
ZorbistHash::HashBlackToMove(const u64& oldHash) const
{
    return oldHash ^ black_to_move;
}

u64
ZorbistHash::HashCastling(const u64& oldHash, const u8 castlingState) const
{
    u64 hash = oldHash;
    if ((castlingState & 1) == 1)
        hash ^= castling[0];
    if ((castlingState & 2) == 2)
        hash ^= castling[1];
    if ((castlingState & 4) == 4)
        hash ^= castling[2];
    if ((castlingState & 8) == 8)
        hash ^= castling[3];

    return hash;
}
