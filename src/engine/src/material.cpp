#include "material.h"

Material::Material()
{}

Material::Material(const Material& other)
{
	*this = other;
}
Material::Material(Material&& other)
{
	*this = std::move(other);
}

Material& Material::operator=(const Material& other)
{
	m_material = other.m_material;
	return *this;
}

Material& Material::operator=(Material&& other)
{
	m_material = std::move(other.m_material);
	return *this;
}

void Material::AddPiece(const ChessPiece& piece, const Notation& position)
{
	m_material[piece.index()].push_back(position);
}

void Material::RemovePiece(const ChessPiece& piece, const Notation& position)
{
	// find piece by position
	auto& pieces = m_material[piece.index()];
	auto it = std::find_if(pieces.begin(), pieces.end(), [&position](const Notation& n) { return n == position; });
			
	FATAL_ASSERT(it != pieces.end()) << "Piece not found - this should not happen";	
	// erase piece from vector
	pieces.erase(it);	
}

void Material::MovePiece(const ChessPiece& piece, const Notation& source, const Notation& target)
{
	// find piece by position
	auto& pieces = m_material[piece.index()];
	auto it = std::find_if(pieces.begin(), pieces.end(), [&source](const Notation& n) { return n == source; });
		
	FATAL_ASSERT(it != pieces.end()) << "Piece not found - this should not happen";

	// update piece position
	(*it) = target;
}

void Material::PromotePiece(const ChessPiece& piece, const Notation& position)
{
	// find piece by position
	auto& positions = m_material[0];
	auto it = std::find_if(positions.begin(), positions.end(), [&position](const Notation& n) { return n == position; });

	FATAL_ASSERT(it != positions.end()) << "Piece not found - this should not happen";
	
	// since we need to remove the position from the pawn list we need to add it first to the new piece list
	// other wise we'll end up with a broken reference.
	AddPiece(piece, *it);
	RemovePiece(ChessPiece(piece.getSet(), PieceType::PAWN), *it);

}

std::vector<Notation> Material::getPlacementsOfPiece(const ChessPiece& piece) const
{
	return m_material[piece.index()];
}

void 
Material::UnmakePieceMove(const ChessPiece& pieceToAdd, const ChessPiece& pieceToRemove, const Notation& source, const Notation& target)
{
	// piece to add and piece to remove could be different if we are dealing with a promotion
	
	// since unmake does things backwards we're removing the piece from the target
	// and adding it again to the source.

	// remove piece from target
	RemovePiece(pieceToRemove, target);

	// add piece to source
	AddPiece(pieceToAdd, source);
}

u32 Material::getPieceCount(const ChessPiece& piece) const
{
	return m_material[piece.index()].size();
}

u32 Material::getValue() const
{
	u32 value = 0;
	for (u32 i = 0; i < m_material.size(); ++i)
	{
		const auto& positions = m_material[i];
		value += positions.size() * ChessPieceDef::Value(i+1);
	}
	return value;
}

u32 Material::getCount() const
{
	u32 count = 0;
	for (const auto& pieces : m_material)
	{
		count += pieces.size();
	}
	return count;
}