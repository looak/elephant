#include <material/chess_piece.hpp>
#include <iostream>
#include "log.h"

ChessPiece ChessPiece::s_empty = ChessPiece();

Set
ChessPiece::FlipSet(Set source)
{
    int retValue = !(int)source;
    return (Set)retValue;
}

byte
ChessPiece::FlipSet(byte source)
{
    byte retValue = !source;
    return retValue;
}

ChessPiece::ChessPiece() :
    m_internalState(0x00)
{
}

char
ChessPiece::toString() const
{
    char retValue = ' ';
    switch (getType()) {
    case PieceType::PAWN:
        retValue = 'p';
        break;
    case PieceType::BISHOP:
        retValue = 'b';
        break;
    case PieceType::KNIGHT:
        retValue = 'n';
        break;
    case PieceType::ROOK:
        retValue = 'r';
        break;
    case PieceType::QUEEN:
        retValue = 'q';
        break;
    case PieceType::KING:
        retValue = 'k';
        break;
    case PieceType::NONE:
        retValue = ' ';
        break;
    default:
        LOG_ERROR() << "Invalid Chess Piece;\n";
    }

    if (getSet() == Set::WHITE)
        retValue = std::toupper(retValue);

    return retValue;
}

bool
ChessPiece::fromString(char piece)
{
    Set set = Set::BLACK;
    if (std::isupper(piece)) {
        set = Set::WHITE;
    }

    char lower = std::tolower(piece);
    PieceType type;
    switch (lower) {
    case 'p':
        type = PieceType::PAWN;
        break;
    case 'n':
        type = PieceType::KNIGHT;
        break;
    case 'b':
        type = PieceType::BISHOP;
        break;
    case 'r':
        type = PieceType::ROOK;
        break;
    case 'q':
        type = PieceType::QUEEN;
        break;
    case 'k':
        type = PieceType::KING;
        break;
    default:
        m_internalState = 0;
        return false;
    }

    *this = ChessPiece(set, type);
    return true;
}

bool
ChessPiece::operator==(const ChessPiece& rhs) const
{
    return m_internalState == rhs.m_internalState;
}

bool
ChessPiece::operator!=(const ChessPiece& rhs) const
{
    return !(*this == rhs);
}

bool
ChessPiece::operator<(const ChessPiece& rhs) const
{
    return m_internalState < rhs.m_internalState;
}