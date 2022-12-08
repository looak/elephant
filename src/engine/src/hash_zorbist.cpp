#include "hash_zorbist.h"
#include "chessboard.h"

// https://stackoverflow.com/questions/33010010/how-to-generate-random-64-bit-unsigned-integer-in-c
#define IMAX_BITS(m) ((m)/((m)%255+1) / 255%255*8 + 7-86/((m)%255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)
_Static_assert((RAND_MAX& (RAND_MAX + 1u)) == 0, "RAND_MAX not a Mersenne number");

u64 rand64(void) {
	u64 r = 0;
	for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
		r <<= RAND_MAX_WIDTH;
		r ^= (unsigned)rand();
	}
	return r;
}

void ZorbistHash::GenerateZorbistTable()
{
	if (initialized)
		LOG_ERROR() << "Zorbist Hash Table has already been initialized!";

	black_to_move = rand64();
	for (u8 i = 0; i < 64; ++i)
	{
		for (u8 p = 0; p < 12; ++p)
		{
			table[i][p] = rand64();
		}
	}

	initialized = true;
}

u64 ZorbistHash::HashBoard(const Chessboard& board)
{
	u64 hash = 0;
	auto itr = board.begin();

	while (itr != board.end())
	{
		if ((*itr).readPiece().isValid())
		{
			u8 pieceIndx = static_cast<u8>((*itr).readPiece().getType()) - 1;
			hash ^= table[itr.index()][pieceIndx];
		}

		itr++;
	}

	return hash;
}
