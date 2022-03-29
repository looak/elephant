#include "chessboard.h"
#include "move.h"
#include "log.h"

ChessboardTile::ChessboardTile(Notation&& notation) :
	m_position(std::move(notation))
{
}

bool ChessboardTile::operator==(const ChessboardTile& rhs) const
{
	bool result = m_position == rhs.m_position;
	result |= m_piece == rhs.m_piece;
	return result;
}

Chessboard::Chessboard()
{
	for (byte r = 0; r < 8; r++)
	{
		for (byte f = 0; f < 8; f++)
		{
			Notation pos(r,f);
			m_tiles[pos.index()].editPosition() = std::move(pos);
		}
	}
}

bool Chessboard::PlacePiece(const ChessPiece& piece, const Notation& target)
{
	if (!Bitboard::IsValidSquare(target))
		return false;

	const auto& tsqrPiece = m_tiles[target.index()].readPiece(); // should we do this check on the bitboard instead?
	if (tsqrPiece != ChessPiece())
		return false; // already a piece on this square

	m_tiles[target.index()].editPiece() = piece;
	m_bitboard.PlacePiece(piece, target);
	return true;
}

bool Chessboard::UpdateEnPassant(const Notation& source, const Notation& target, bool wasPawnMove)
{
	m_enPassant = Notation();

	if (wasPawnMove)
	{
		signed char dif = source.rank - target.rank;
		dif = abs(dif);
		if (dif == 2) // we made a enpassant move
		{
			m_enPassant = Notation(source.file, source.rank + 1);
			return true;
		}
	}

	return false;
}

void Chessboard::InternalMakeMove(const Notation& source, const Notation& target)
{
	ChessPiece piece = m_tiles[source.index()].editPiece();
	m_tiles[source.index()].editPiece() = ChessPiece(); // clear old square.
	m_tiles[target.index()].editPiece() = piece;

	m_bitboard.ClearPiece(piece, source);
	m_bitboard.PlacePiece(piece, target);
}

bool Chessboard::MakeMove(Move& move)
{
	move.Flags = MoveFlag::Invalid;
	
	if (!Bitboard::IsValidSquare(move.SourceSquare) || !Bitboard::IsValidSquare(move.TargetSquare))
		return false;

	const auto& piece = m_tiles[move.SourceSquare.index()].readPiece();
	if (piece == ChessPiece())
		return false;

	if (m_bitboard.IsValidMove(move.SourceSquare, piece, move.TargetSquare) == false)
		return false;

		
	move.Flags = MoveFlag::Zero;
	move.Piece = piece;
	bool isPawn = piece.getType() == PieceType::PAWN;

	if (m_tiles[move.TargetSquare.index()].readPiece() != ChessPiece())
	{
		move.Flags |= MoveFlag::Capture;
	}

	// do move
	InternalMakeMove(move.SourceSquare, move.TargetSquare);
	if (UpdateEnPassant(move.SourceSquare, move.TargetSquare, isPawn))
		move.Flags |= MoveFlag::EnPassant;

	return true;
}

const ChessboardTile&
Chessboard::readTile(const Notation& position) const
{
	return m_tiles[position.index()];
}

ChessboardTile& 
Chessboard::editTile(const Notation& position)
{
	return m_tiles[position.index()];
}

const Notation s_beginPos = Notation::BuildPosition('a', 1);
const Notation s_endPos = Notation(0, 8);

Chessboard::Iterator
Chessboard::begin()
{
	return Chessboard::Iterator(*this, Notation(s_beginPos));
}

Chessboard::Iterator 
Chessboard::end()
{
	return Chessboard::Iterator(*this, Notation(s_endPos));
}
Chessboard::ConstIterator 
Chessboard::begin() const
{
	return Chessboard::ConstIterator(*this, Notation(s_beginPos));
}
Chessboard::ConstIterator
Chessboard::end() const
{
	return Chessboard::ConstIterator(*this, Notation(s_endPos));
}