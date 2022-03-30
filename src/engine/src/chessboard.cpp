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

Chessboard::Chessboard() :
	m_castlingState(0),
	m_enPassant(Notation()),
	m_enPassantTarget(Notation())
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

bool Chessboard::UpdateEnPassant(const Notation& source, const Notation& target)
{
	signed char dif = source.rank - target.rank;
	if (abs(dif) == 2) // we made a enpassant move
	{
		dif *= .5f;
		m_enPassant = Notation(source.file, source.rank - dif);
		m_enPassantTarget = Notation(target);
		return true;
	}

	return false;
}

Notation Chessboard::InternalHandlePawnMove(Move& move)
{
	auto pieceTarget = Notation(move.TargetSquare);	
	// compare target square with en passant - if this is equal we build a "offset target" where the pawn should be.
	if (pieceTarget == m_enPassant)
	{
		pieceTarget = Notation(m_enPassantTarget);
		if (m_tiles[pieceTarget.index()].readPiece().getType() != PieceType::PAWN)
			LOG_ERROR() << "No Pawn in expected EnPassant target square!";
	}

	// reset enpassant cached values
	m_enPassant = Notation();
	m_enPassantTarget = Notation();

	if (UpdateEnPassant(move.SourceSquare, move.TargetSquare))
	{
		move.Flags |= MoveFlag::EnPassant;
	}

	// we need to flip set for this calculation since it's the opposite side of the
	// board which is the promotion rank.
	byte promoteRankCheck = 7 * ChessPiece::FlipSet(move.Piece.set());
	if (move.TargetSquare.rank == promoteRankCheck)
	{ // edit the source tile piece, since we're using this when we do our internal move.
		m_tiles[move.SourceSquare.index()].editPiece() = move.Promote;
		move.Flags |= MoveFlag::Promotion;
	}

	return pieceTarget;
}

void Chessboard::InternalHandleKingMove(Move& move, Notation& targetRook, Notation& rookMove)
{
	byte casltingMask = 3 << (2 * move.Piece.set());
	if (m_castlingState & casltingMask)
	{
		byte targetRank = 7 * move.Piece.set();
		if (move.TargetSquare.file == 2) // we are in c file.
		{
			targetRook = Notation(0, targetRank);
			rookMove = Notation(3, targetRank);
			
		}
		else if (move.TargetSquare.file == 6) // we are in g file.
		{
			targetRook = Notation(7, targetRank);
			rookMove = Notation(5, targetRank);
		}

		move.Flags |= MoveFlag::Castle;
	}
	casltingMask &= m_castlingState;
	m_castlingState ^= casltingMask;
}

void Chessboard::InternalHandleRookMove(Move& move, const Notation& targetRook, const Notation& rookMove)
{  	
	if (move.Piece.getType() == PieceType::KING && targetRook != Notation())
	{
		InternalMakeMove(targetRook, rookMove);
	}
	else
	{
		byte mask = 0;
		// 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
		switch(move.SourceSquare.index())
		{
			case 63: // H8 Black King Side Rook
				mask |= 0x04;
				break;
			case 56: // A8 Black Queen Side Rook
				mask |= 0x08;
				break;
			case 7: // H1 White King Side Rook
				mask |= 0x01;
				break;
			case 0: // A1 White Queen Side Rook
				mask |= 0x02;
				break;
		}

		mask &= m_castlingState;
		m_castlingState ^= mask;
	}
}

void Chessboard::InternalHandleKingRookMove(Move& move)
{
	Notation targetRook, rookMove;
	switch(move.Piece.getType())
	{
		case PieceType::KING:
		InternalHandleKingMove(move, targetRook, rookMove);
		
		case PieceType::ROOK:
		InternalHandleRookMove(move, targetRook, rookMove);

		default:
		break;
	}
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

	if (m_bitboard.IsValidMove(move.SourceSquare, piece, move.TargetSquare, m_castlingState, m_enPassant.index()) == false)
		return false;
	
	move.Flags = MoveFlag::Zero;
	move.Piece = piece;

	auto pieceTarget = Notation(move.TargetSquare);

	switch(piece.getType())
	{
		case PieceType::PAWN:
		pieceTarget = InternalHandlePawnMove(move);
		break;
		
		case PieceType::KING:
		case PieceType::ROOK:
		InternalHandleKingRookMove(move);

		default:
		// reset enpassant cached values
		m_enPassant = Notation();
		m_enPassantTarget = Notation();
	}

	if (m_tiles[pieceTarget.index()].readPiece() != ChessPiece())
	{
		move.Flags |= MoveFlag::Capture;
		// remove captured piece from board.
		m_tiles[pieceTarget.index()].editPiece() = ChessPiece();
	}

	// do move
	InternalMakeMove(move.SourceSquare, move.TargetSquare);

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