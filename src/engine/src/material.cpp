#include "material.h"
#include <algorithm>
#include "bitboard_constants.h"
#include "intrinsics.hpp"

Material::Material() { Clear(); }

Material::Material(const Material& other) { *this = other; }

Material& Material::operator=(const Material& other)
{
    m_materialBitboards = other.m_materialBitboards;
    return *this;
}

void Material::Clear()
{
    for (u64& bb : m_materialBitboards) {
        bb = 0;
    }
}

void Material::AddPiece(ChessPiece piece, Notation position)
{
    FATAL_ASSERT((m_materialBitboards[piece.index()] & squareMaskTable[position.index()]) == 0)
        << "There is already a piece on given square.";

    m_materialBitboards[piece.index()] |= squareMaskTable[position.index()];
    // m_material[piece.index()].push_back(position);
}

void Material::RemovePiece(ChessPiece piece, Notation position)
{
    FATAL_ASSERT((m_materialBitboards[piece.index()] & squareMaskTable[position.index()]) > 0)
        << "There is no piece on given square.";

    m_materialBitboards[piece.index()] &= ~squareMaskTable[position.index()];
}

void Material::MovePiece(ChessPiece piece, Notation source, Notation target)
{
    RemovePiece(piece, source);
    AddPiece(piece, target);
}

void Material::PromotePiece(ChessPiece piece, Notation position)
{
    AddPiece(piece, position);
    RemovePiece(ChessPiece(piece.getSet(), PieceType::PAWN), position);
}

std::vector<Notation> Material::buildPlacementsOfPiece(ChessPiece piece) const
{
    u64 pieceBitboard = readPieceBitboard(piece.index());
    std::vector<Notation> placements;
    while (pieceBitboard > 0) {
        u32 sqr = readNextPiece(pieceBitboard);
        placements.push_back(Notation(sqr));
    }
    return placements;
}

u64 Material::readPieceBitboard(PieceType piece) const { return readPieceBitboard((u32)piece - 1); }
u64 Material::readPieceBitboard(u32 pieceIndex) const { return m_materialBitboards[pieceIndex]; }

u32 Material::readNextPiece(u64& pieceBitboard) const
{
    i32 lsb = intrinsics::lsbIndex(pieceBitboard);
    pieceBitboard = intrinsics::resetLsb(pieceBitboard);
    return (u32)lsb;
}

void Material::UnmakePieceMove(ChessPiece pieceToAdd,
                               ChessPiece pieceToRemove,
                               Notation source,
                               Notation target)
{
    // piece to add and piece to remove could be different if we are dealing with a promotion

    // since unmake does things backwards we're removing the piece from the target
    // and adding it again to the source.

    // remove piece from target
    RemovePiece(pieceToRemove, target);

    // add piece to source
    AddPiece(pieceToAdd, source);
}

u32 Material::getPieceCount(ChessPiece piece) const
{
    return intrinsics::popcnt(m_materialBitboards[piece.index()]);
}

u32 Material::getPieceCount(PieceType type) const
{
    return intrinsics::popcnt(m_materialBitboards[(u32)type - 1]);
}

u32 Material::getValue() const
{
    u32 value = 0;
    for (u32 pieceIndex = 0; pieceIndex < pieceIndexMax; ++pieceIndex) {
        u32 pieceCount = intrinsics::popcnt(m_materialBitboards[pieceIndex]);
        value += pieceCount * ChessPieceDef::Value(pieceIndex);
    }
    return value;
}

u32 Material::getCount() const
{
    u32 count = 0;
    for (u64 bb : m_materialBitboards) {
        count += intrinsics::popcnt(bb);
    }
    return count;
}