#include "material.h"
#include <algorithm>

Material::Material()
{}

Material::Material(const Material& other)
{
	*this = other;
}

Material& Material::operator=(const Material& other)
{
	m_material = other.m_material;
	return *this;
}

void Material::Clear()
{
	// for each piece
	for (auto& pieceList : m_material)
	{
		pieceList.clear();
	}
}

void Material::AddPiece(ChessPiece piece, Notation position)
{
	m_material[piece.index()].push_back(position);
}

void Material::RemovePiece(ChessPiece piece, Notation position)
{
	// find piece by position
	auto& pieces = m_material[piece.index()];
	auto it = std::find_if(pieces.begin(), pieces.end(), [&position](Notation n) { return n == position; });
			
	FATAL_ASSERT(it != pieces.end()) << "Piece not found - this should not happen: " << piece.toString() << ":" << position.toString();
	// erase piece from vector
	pieces.erase(it);
}

void Material::MovePiece(ChessPiece piece, Notation source, Notation target)
{
	// find piece by position
	auto& pieces = m_material[piece.index()];
	auto it = std::find_if(pieces.begin(), pieces.end(), [&source](Notation n) { return n == source; });
		
	FATAL_ASSERT(it != pieces.end()) << "Piece not found - this should not happen: " << piece.toString() << ":" << source.toString() << "->" << target.toString();

	// update piece position
	(*it) = target;
}

void Material::PromotePiece(ChessPiece piece, Notation position)
{
	// find piece by position
	auto& positions = m_material[0];
	auto it = std::find_if(positions.begin(), positions.end(), [&position](Notation n) { return n == position; });

	FATAL_ASSERT(it != positions.end()) << "Piece not found - this should not happen";
	
	// since we need to remove the position from the pawn list we need to add it first to the new piece list
	// other wise we'll end up with a broken reference.
	AddPiece(piece, *it);
	RemovePiece(ChessPiece(piece.getSet(), PieceType::PAWN), *it);
}

const std::vector<Notation>& Material::getPlacementsOfPiece(ChessPiece piece) const
{
	return m_material[piece.index()];
}

void 
Material::UnmakePieceMove(ChessPiece pieceToAdd, ChessPiece pieceToRemove, Notation source, Notation target)
{
	// piece to add and piece to remove could be different if we are dealing with a promotion
	
	// since unmake does things backwards we're removing the piece from the target
	// and adding it again to the source.

	// remove piece from target
	RemovePiece(pieceToRemove, target);

	// add piece to source
	AddPiece(pieceToAdd, source);
}

size_t Material::getPieceCount(ChessPiece piece) const
{
	return m_material[piece.index()].size();
}

size_t Material::getPieceCount(PieceType type) const
{
    return m_material[(size_t)type].size();
}

u32 Material::getValue() const
{
	u32 value = 0;
	for (u32 i = 0; i < m_material.size(); ++i)
	{
		const auto& positions = m_material[i];
		value += (u32)positions.size() * ChessPieceDef::Value(i);
	}
	return value;
}

size_t Material::getCount() const
{
	size_t count = 0;
	for (const auto& pieces : m_material)
	{
		count += pieces.size();
	}
	return count;
}