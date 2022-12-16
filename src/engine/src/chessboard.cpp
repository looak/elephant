#include "chessboard.h"
#include "move.h"
#include "log.h"
#include "hash_zorbist.h"

ChessboardTile::ChessboardTile(Notation&& notation) :
	m_position(std::move(notation))
{
}

bool 
ChessboardTile::operator==(const ChessboardTile& rhs) const
{
	bool result = m_position == rhs.m_position;
	result |= m_piece == rhs.m_piece;
	return result;
}

ChessboardTile&
ChessboardTile::operator=(const ChessboardTile& rhs)
{
	m_position = Notation(rhs.m_position);
	m_piece = rhs.m_piece;

	return *this;
}

Chessboard::Chessboard() :
	m_hash(0),
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

	m_kings[0].first = ChessPiece();
	m_kings[0].second = Notation();
	m_kings[1].first = ChessPiece();
	m_kings[1].second = Notation();
}

Chessboard::Chessboard(const Chessboard& other) :
	m_hash(other.readHash()),
	m_castlingState(other.readCastlingState()),
	m_enPassant(other.readEnPassant()),
	m_enPassantTarget(other.m_enPassantTarget)
{
	auto otherItr = other.begin();
	auto thisItr = this->begin();
	while (otherItr != other.end())
	{
		*thisItr = *otherItr;
		otherItr++;
		thisItr++;
	}

	m_kings[0].first = other.m_kings[0].first;
	m_kings[0].second = Notation(other.m_kings[0].second);
	m_kings[1].first = other.m_kings[1].first;
	m_kings[1].second = Notation(other.m_kings[1].second);

	m_bitboard = other.m_bitboard;
}

bool 
Chessboard::PlacePiece(const ChessPiece& piece, const Notation& target, bool overwrite)
{
	if (!Bitboard::IsValidSquare(target))
		return false;

	const auto& tsqrPiece = m_tiles[target.index()].readPiece();
	if (tsqrPiece != ChessPiece())
	{
		if (overwrite == true)
			m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, tsqrPiece, target);
		else
			return false; // already a piece on this square
	}

	if (piece.getType() == PieceType::KING)
	{
		m_kings[piece.set()].first = piece;
		m_kings[piece.set()].second = Notation(target);
	}

	m_tiles[target.index()].editPiece() = piece;
	m_bitboard.PlacePiece(piece, target);

	m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, piece, target);
	return true;
}

bool 
Chessboard::UpdateEnPassant(const Notation& source, const Notation& target)
{
	signed char dif = source.rank - target.rank;
	if (abs(dif) == 2) // we made a enpassant move
	{
		dif *= .5f;
		m_enPassant = Notation(source.file, source.rank - dif);
		m_enPassantTarget = Notation(target);
		m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);
		return true;
	}

	return false;
}

Notation 
Chessboard::InternalHandlePawnMove(Move& move)
{
	auto pieceTarget = Notation(move.TargetSquare);	
	// compare target square with en passant - if this is equal we build a "offset target" where the pawn should be.
	if (pieceTarget == m_enPassant)
	{
		// since our move was the same as the enPassant we're capturing this piece in enpassant
		move.Flags |= MoveFlag::EnPassant;

		pieceTarget = Notation(m_enPassantTarget);
		if (m_tiles[pieceTarget.index()].readPiece().getType() != PieceType::PAWN)
			LOG_ERROR() << "No Pawn in expected EnPassant target square!";
	}	

	if (Notation::Validate(m_enPassant))
		m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);
	// reset enpassant cached values
	m_enPassant = Notation();
	m_enPassantTarget = Notation();

	UpdateEnPassant(move.SourceSquare, move.TargetSquare);

	if (IsPromoting(move))
	{ // edit the source tile piece, since we're using this when we do our internal move.
		m_tiles[move.SourceSquare.index()].editPiece() = move.Promote;
		move.Flags |= MoveFlag::Promotion;
	}

	return pieceTarget;
}

void 
Chessboard::InternalHandleKingMove(Move& move, Notation& targetRook, Notation& rookMove)
{
	byte casltingMask = 3 << (2 * move.Piece.set());
	if (m_castlingState & casltingMask)
	{
		byte targetRank = 7 * move.Piece.set();
		if (move.TargetSquare.file == 2) // we are in c file.
		{
			targetRook = Notation(0, targetRank);
			rookMove = Notation(3, targetRank);
			move.Flags |= MoveFlag::Castle;
			
		}
		else if (move.TargetSquare.file == 6) // we are in g file.
		{
			targetRook = Notation(7, targetRank);
			rookMove = Notation(5, targetRank);
			move.Flags |= MoveFlag::Castle;
		}
	}
	m_hash = ZorbistHash::Instance().HashCastling(m_hash, m_castlingState);
	casltingMask &= m_castlingState;
	m_castlingState ^= casltingMask;
	m_hash = ZorbistHash::Instance().HashCastling(m_hash, m_castlingState);
}

void 
Chessboard::InternalHandleRookMove(Move& move, const Notation& targetRook, const Notation& rookMove)
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

		m_hash = ZorbistHash::Instance().HashCastling(m_hash, m_castlingState);
		mask &= m_castlingState;
		m_castlingState ^= mask;
		m_hash = ZorbistHash::Instance().HashCastling(m_hash, m_castlingState);
	}
}

void 
Chessboard::InternalHandleKingRookMove(Move& move)
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

void 
Chessboard::InternalMakeMove(const Notation& source, const Notation& target)
{
	ChessPiece piece = m_tiles[source.index()].editPiece();
	
	if (piece.getType() == PieceType::KING)
		m_kings[piece.set()].second = Notation(target);

	m_tiles[source.index()].editPiece() = ChessPiece(); // clear old square.
	m_tiles[target.index()].editPiece() = piece;

	m_bitboard.ClearPiece(piece, source);
	m_bitboard.PlacePiece(piece, target);
	
	// update hash
	m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, piece, target);
	m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, piece, source);
}

bool
Chessboard::VerifyMove(const Move& move) const
{
	if (!Bitboard::IsValidSquare(move.SourceSquare) || !Bitboard::IsValidSquare(move.TargetSquare))
		return false;

	const auto& piece = m_tiles[move.SourceSquare.index()].readPiece();
	if (piece == ChessPiece())
		return false;

	u64 threatenedMask = GetThreatenedMask(ChessPiece::FlipSet(move.Piece.getSet()));

	if (m_bitboard.IsValidMove(move.SourceSquare, piece, move.TargetSquare, m_castlingState, m_enPassant.index(), threatenedMask) == false)
		return false;
	
	return true;
}

bool
Chessboard::UnmakeMove(const Move& move)
{
	if (Notation::Validate(m_enPassant))
		m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);

	m_enPassant = Notation();
	m_enPassantTarget = Notation();

	if (move.EnPassantTargetSquare.isValid())
	{
		byte offset = move.Piece.getSet() == Set::WHITE ? 1 : -1;
		m_enPassant = Notation(move.EnPassantTargetSquare.file, move.EnPassantTargetSquare.rank + offset);
		m_enPassantTarget = move.EnPassantTargetSquare;
		m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);
	}
	
	auto& sourceTile = m_tiles[move.SourceSquare.index()];
	auto& targetTile = m_tiles[move.TargetSquare.index()];

	// unmake chessboard
	sourceTile.editPiece() = targetTile.readPiece();
	targetTile.editPiece() = ChessPiece();

	// unmake bitboard
	m_bitboard.ClearPiece(move.Piece, move.TargetSquare);
	m_bitboard.PlacePiece(move.Piece, move.SourceSquare);

	if (move.Capture.isValid())
	{
		if ((move.Flags & MoveFlag::EnPassant) == MoveFlag::EnPassant)
		{
			byte offset = move.Piece.getSet() == Set::WHITE ? -1 : 1;
			m_enPassantTarget = Notation(move.TargetSquare.file, move.TargetSquare.rank + offset);
			m_enPassant = move.TargetSquare;

			m_tiles[m_enPassantTarget.index()].editPiece() = move.Capture;
			m_bitboard.PlacePiece(move.Capture, m_enPassantTarget);
			m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, move.Capture, m_enPassantTarget);
			m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);

		}
		else
		{
			targetTile.editPiece() = move.Capture;
			m_bitboard.PlacePiece(move.Capture, move.TargetSquare);
			m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, move.Capture, move.TargetSquare);
		}
	}

	// update hash
	m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, move.Piece, move.TargetSquare);
	m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, move.Piece, move.SourceSquare);

	return true;
}

Move
Chessboard::MakeMove(const Move& move)
{
	auto boarditr = begin();
	int increment = 1;
	// figure out the source of this move.
	/*if (Notation::Validate(move.SourceSquare))
	{
		u64 availableMoves = m_bitboard.GetAvailableMoves(move.SourceSquare, move.Piece, m_castlingState, m_enPassant.index());
		u64 targetMask = UINT64_C(1) << move.TargetSquare.index();

		if (availableMoves & targetMask)
		{
			Move actualMove(move.SourceSquare, move.TargetSquare);
			MakeMove(actualMove);
			return actualMove;
		}
	}
	else*/ if (move.SourceSquare.file == 9) // the move is ambigious and we have extra information about it.
	{
		boarditr += (move.SourceSquare.rank * 8);
	}
	else if (move.SourceSquare.rank == 9)
	{
		boarditr += (move.SourceSquare.file);
		increment = 8;
	}

	while (boarditr != end())
	{
		auto tile = *boarditr;
		if (move.Piece == tile.readPiece())
		{
			u64 availableMoves = m_bitboard.GetAvailableMoves(tile.readPosition(), move.Piece, m_castlingState, m_enPassant.index());
			availableMoves |= m_bitboard.GetAttackedSquares(tile.readPosition(), move.Piece);
			u64 targetMask = UINT64_C(1) << move.TargetSquare.index();

			if (availableMoves & targetMask)
			{
				Move actualMove(tile.readPosition(), move.TargetSquare);
				MakeMove(actualMove);
				return actualMove;
			}
		}

		boarditr += increment;
	}
}

bool 
Chessboard::MakeMove(Move& move)
{
	move.Flags = MoveFlag::Invalid;
	
	if (!VerifyMove(move))
		return false;

	const auto& piece = m_tiles[move.SourceSquare.index()].readPiece();
	move.Flags = MoveFlag::Zero;
	move.Piece = piece;

	auto pieceTarget = Notation(move.TargetSquare);

	// storing enpassant target square so we can unmake this.
	move.EnPassantTargetSquare = m_enPassantTarget;

	switch(piece.getType())
	{
		case PieceType::PAWN:
		// updating pieceTarget since if we're capturing enpassant the target will be on a different square.
		pieceTarget = InternalHandlePawnMove(move);
		break;
		
		case PieceType::KING:
		case PieceType::ROOK:
		InternalHandleKingRookMove(move);

		default:
		// reset enpassant cached values
		if (Notation::Validate(m_enPassant))
			m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);

		m_enPassant = Notation();
		m_enPassantTarget = Notation();
	}

	if (m_tiles[pieceTarget.index()].readPiece() != ChessPiece())
	{
		move.Flags |= MoveFlag::Capture;
		move.Capture = m_tiles[pieceTarget.index()].readPiece();
		// remove captured piece from board.
		m_tiles[pieceTarget.index()].editPiece() = ChessPiece();
		
		// remove piece from hash
		m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, move.Capture, pieceTarget);
	}

	if (IsCheck(move))
		move.Flags |= MoveFlag::Check;

	// do move
	InternalMakeMove(move.SourceSquare, move.TargetSquare);

	return true;
}

bool
Chessboard::IsMoveCastling(const Move& move) const
{
	byte casltingMask = 3 << (2 * move.Piece.set());
	if (m_castlingState & casltingMask)
	{
		if (move.TargetSquare.file == 2) 
			return true; // castling queen side
		else if (move.TargetSquare.file == 6)
			return true; // castling king side
	}

	return false;
}

bool
Chessboard::IsPromoting(const Move& move) const
{
	// we need to flip set for this calculation since it's the opposite side of the
	// board which is the promotion rank.
	byte promoteRankCheck = 7 * ChessPiece::FlipSet(move.Piece.set());
	if (move.TargetSquare.rank == promoteRankCheck)
		return true;

	return false;
}

bool
Chessboard::Checked(Set set) const
{
	u8 indx = static_cast<u8>(set);
	auto king = m_kings[indx];

	if (king.first == ChessPiece())
		return false;

	Set op = ChessPiece::FlipSet(set);
	u64 threatMask = GetThreatenedMask(op);
	u64 kingMask = UINT64_C(1) << king.second.index();

	if (threatMask & kingMask)
		return true;
	
	return false;
}

bool
Chessboard::IsCheck(const Move& move) const
{
	u64 threatened = ~universe;
	// get threatened squares from new location
	auto flag = (move.Flags & MoveFlag::Promotion);
	if (flag == MoveFlag::Promotion)
		threatened = m_bitboard.GetThreatenedSquares(move.TargetSquare, move.Promote);
	else
		threatened = m_bitboard.GetThreatenedSquares(move.TargetSquare, move.Piece);
	
	auto opSet = ChessPiece::FlipSet(move.Piece.set());
	
	if (!m_bitboard.IsValidSquare(m_kings[opSet].second))
		return false;

	u64 kingMask = UINT64_C(1) << m_kings[opSet].second.index();

	if (threatened & kingMask)
		return true;
	
	return false;
}

u64 Chessboard::GetKingMask(Set set) const
{
	u8 indx = static_cast<u8>(set);
	if (m_kings[indx].first == ChessPiece())
		return 0;

	u64 opSlidingMask = GetSlidingMask(ChessPiece::FlipSet(set));
	return m_bitboard.GetKingMask(m_kings[indx].first, m_kings[indx].second, opSlidingMask);
}

u64
Chessboard::GetThreatenedMask(Set set) const
{
	u64 mask = ~universe;
	auto boardItr = begin();
	while (boardItr != end())
	{
		const auto& piece = (*boardItr).readPiece();
		const auto& pos = (*boardItr).readPosition();
		if (piece != ChessPiece() && piece.getSet() == set)
			mask |= m_bitboard.GetThreatenedSquares(pos, piece);

		++boardItr;
	}

	return mask;
}

u64 Chessboard::GetSlidingMask(Set set) const
{
	u64 mask = ~universe;
	auto boardItr = begin();
	while (boardItr != end())
	{
		const auto& piece = (*boardItr).readPiece();
		const auto& pos = (*boardItr).readPosition();
		if (piece.isValid() && piece.getSet() == set && piece.isSliding())
			mask |= m_bitboard.GetThreatenedSquares(pos, piece);

		++boardItr;
	}

	return mask;
}


std::vector<Move>
Chessboard::GetAvailableMoves(Set currentSet) const
{
	bool isChecked = Checked(currentSet);
	u64 threatenedMask = GetThreatenedMask(ChessPiece::FlipSet(currentSet));

	u64 kingMask = GetKingMask(currentSet);

	std::vector<Move> result;
	for (auto tile : m_tiles)
	{
		if (tile.readPiece().isValid() && tile.readPiece().getSet() == currentSet)
		{
			auto moves = GetAvailableMoves(tile.readPosition(), tile.readPiece(), threatenedMask, isChecked, kingMask);
			result.insert(result.end(),moves.begin(), moves.end());
		}
	}
	return result;
}

std::vector<Move> 
Chessboard::GetAvailableMoves(const Notation& source, const ChessPiece& piece, u64 threatenedMask, bool checked, u64 kingMask) const
{
	std::vector<Move> moveVector;
	if (!Bitboard::IsValidSquare(source))
		return moveVector;

	if (piece == ChessPiece())
		return moveVector;
	
	u64 movesbb = m_bitboard.GetAvailableMoves(source, piece, m_castlingState, m_enPassant.index(), threatenedMask, checked, kingMask);
	u64 attacked = m_bitboard.GetAttackedSquares(source, piece);

	if (movesbb == 0)
		return moveVector;

	for (signed char rank = 7; rank >= 0; --rank)
	{
		byte byte_rank = static_cast<byte>(rank);
		for(byte file = 0; file < 8; ++file)
		{
			byte sqr = (byte_rank * 8) + file;
			u64 sqrMask = UINT64_C(1) << sqr;

			if (sqrMask & movesbb)
			{				
				auto& move = moveVector.emplace_back(source, Notation(file, rank));
				move.Flags = MoveFlag::Zero;
				move.Piece = piece;

				// I don't think I need to check if we're in check here since I'll be doing the move and unmaking it again to verify
				// this on the board.
				/*if (IsCheck(move))
					move.Flags |= MoveFlag::Check;*/

				/*if (sqrMask & attacked)
					move.Flags |= MoveFlag::Capture;*/

				if (move.Piece.getType() == PieceType::KING && IsMoveCastling(move))
					move.Flags |= MoveFlag::Castle;

				if (piece.getType() == PieceType::PAWN && IsPromoting(move))
				{
					move.Flags |= MoveFlag::Promotion;
					move.Promote = ChessPiece(piece.getSet(), PieceType::QUEEN);

					Move rookPromote = Move(move);
					rookPromote.Promote = ChessPiece(piece.getSet(), PieceType::ROOK);
					
					/*if (IsCheck(rookPromote))
						rookPromote.Flags |= MoveFlag::Check;*/

					Move bishopPromote = Move(move);
					bishopPromote.Promote = ChessPiece(piece.getSet(), PieceType::BISHOP);

					/*if (IsCheck(bishopPromote))
						bishopPromote.Flags |= MoveFlag::Check;*/

					Move knightPromote = Move(move);
					knightPromote.Promote = ChessPiece(piece.getSet(), PieceType::KNIGHT);
					
					/*if (IsCheck(knightPromote))
						knightPromote.Flags |= MoveFlag::Check;

					if (IsCheck(move))
						move.Flags |= MoveFlag::Check;*/

					// when pushing back new elements to the vector our reference to move is moved and 
					// pointing at garbage. So we do this last.
					moveVector.push_back(rookPromote);
					moveVector.push_back(bishopPromote);
					moveVector.push_back(knightPromote);
				}
			}
		}
	}

	return moveVector;
}

bool Chessboard::setEnPassant(const Notation& notation)
{
	u64 newHash = m_hash;
	if (Notation::Validate(m_enPassant))
		newHash = ZorbistHash::Instance().HashEnPassant(newHash, m_enPassant);

	newHash = ZorbistHash::Instance().HashEnPassant(newHash, notation);
	m_enPassant = Notation(notation);
	m_hash = newHash;
	return true;
}

bool Chessboard::setCastlingState(u8 castlingState)
{
	m_hash = ZorbistHash::Instance().HashCastling(m_hash, m_castlingState);
	m_hash = ZorbistHash::Instance().HashCastling(m_hash, castlingState);
	m_castlingState = castlingState;
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