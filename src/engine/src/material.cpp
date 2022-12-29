#include "material.h"

ChessPieceInfo::ChessPieceInfo(const ChessPiece& piece)
	: m_piece(piece)
{}
ChessPieceInfo::ChessPieceInfo(const ChessPiece& piece, const Notation& notation)
	: m_piece(piece), m_position(notation)
{}

ChessPieceInfo::ChessPieceInfo(const ChessPieceInfo& other)
	: m_piece(other.m_piece)
{}


ChessPieceInfo::ChessPieceInfo(ChessPieceInfo&& other)
{
	*this = std::move(other);
}

ChessPieceInfo& ChessPieceInfo::operator=(ChessPieceInfo&& other)
{
	m_piece = std::move(other.m_piece);
	m_position = std::move(other.m_position);	
	return *this;	
}

ChessPieceInfo& ChessPieceInfo::operator=(const ChessPieceInfo& other)
{
	m_piece = other.m_piece;
	m_position = other.m_position;
	return *this;
}

const ChessPiece& ChessPieceInfo::readPiece() const
{
	return m_piece;
}

const Notation& ChessPieceInfo::readPosition() const
{
	return m_position;
}

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
	m_material[piece.index()].push_back(ChessPieceInfo(piece, position));
}

void Material::RemovePiece(const ChessPiece& piece, const Notation& position)
{
	// find piece by position
	auto& pieces = m_material[piece.index()];
	auto it = std::find_if(pieces.begin(), pieces.end(), [&position](const ChessPieceInfo& info) {
		return info.readPosition() == position;
		});
		
	FATAL_ASSERT(it != pieces.end()) << "Piece not found - this should not happen";	
	// erase piece from vector
	pieces.erase(it);	
}

void Material::MovePiece(const ChessPiece& piece, const Notation& source, const Notation& target)
{
	// find piece by position
	auto& pieces = m_material[piece.index()];
	auto it = std::find_if(pieces.begin(), pieces.end(), [&source](const ChessPieceInfo& info) {
		return info.readPosition() == source;
		});
	
	FATAL_ASSERT(it != pieces.end()) << "Piece not found - this should not happen";

	// update piece position
	it->editPosition() = target;	
}

std::vector<Notation> Material::getPlacementsOfPiece(const ChessPiece& piece) const
{
	// build array of positions from material of piece
	std::vector<Notation> positions;
	const auto& pieces = m_material[piece.index()];
	for (const auto& piece : pieces)
	{
		positions.push_back(piece.readPosition());
	}
	return positions;
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
		const auto& pieces = m_material[i];
		for (const auto& piece : pieces)
		{
			value += ChessPieceDef::Value(piece.readPiece().type());
		}
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