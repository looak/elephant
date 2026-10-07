#include <core/chessboard.hpp>
#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <system/platform.hpp>
#include <diagnostics/logger.hpp>
#include <move/move.hpp>
#include <move/generation/move_generator.hpp>
#include <position/hash_zobrist.hpp>

#include <future>
#include <thread>
#include <vector>

Chessboard::Chessboard()
{}

Chessboard::Chessboard(const Chessboard& other) :
    m_gameState(other.m_gameState),
    m_position(other.m_position)
{
}

bool Chessboard::compare(const Chessboard& other) const
{
    return m_gameState == other.m_gameState && m_position == other.m_position;
}

bool Chessboard::compare(const Position& other) const
{
    return m_position == other;
}

std::string
Chessboard::toString() const
{
    auto positionReader = m_position.read();
    auto boardItr = positionReader.begin();
    auto endItr = positionReader.end();
    std::array<std::stringstream, 8> ranks;
    byte prevRank = 0xFF;
    do  // build each row
    {
        if (prevRank != boardItr.rank()) {
            ranks[boardItr.rank()] << "\n" << (int)(boardItr.rank() + 1) << "  ";
        }

        ranks[boardItr.rank()] << '[' << boardItr.get().toString() << ']';
        prevRank = boardItr.rank();
        ++boardItr;

    } while (boardItr != endItr);

    std::stringstream boardstream;
    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend())  // rebuild the board
    {
        boardstream << (*rankItr).str();
        rankItr++;
    }

    boardstream << "\n    A  B  C  D  E  F  G  H\n";

    
    boardstream << "castling state: " << positionReader.castling().toString();
    boardstream << "\nen passant: " << positionReader.enPassant().toString();
    boardstream << "\nhash: 0x" << positionReader.hash() << "\n";

    return boardstream.str();
}