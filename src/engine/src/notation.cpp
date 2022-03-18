#include "notation.h"
#include <locale>

Notation Notation::BuildPosition(byte file, byte rank)
{
	Notation result(0xF, 0xF);
	byte corrFile = (byte)(tolower(file) - 'a');
	byte corrRank = rank - 1;

	// validate placement is inside the board.
	if (corrFile > 7 || corrRank > 7)
		return result;

	result.file = corrFile;	
	result.rank = corrRank;
	return result;
}

bool Notation::Validate(const Notation& notation)
{
	// since byte is unsigned we can't have negative values.
	// and if we do it will automatically wrap to a postive value.
	bool validFile = notation.file < 8;
	bool validRank = notation.rank < 8;

	return validFile && validRank;
}

std::string Notation::toString(const Notation& notation)
{
	char output[2];
	output[0] = notation.file + 'a';
	output[1] = notation.rank + '1';
	return std::string(output, 2);
}

Notation& Notation::operator=(Notation&& other)
{
	file = other.file;
	rank = other.rank;
	return *this;
}

bool Notation::operator==(const Notation& rhs) const
{
	bool result = rank == rhs.rank;
	result &= file == rhs.file;
	return result;
}