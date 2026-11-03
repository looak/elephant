#include <core/chessboard.hpp>
#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <system/platform.hpp>
#include <move/move.hpp>
#include <move/move_executor.hpp>
#include <position/hash_zobrist.hpp>

#include <future>
#include <sstream>
#include <thread>
#include <vector>

Chessboard::Chessboard()
{}

template<bool validation>
void Chessboard::makeMove(const PackedMove move, MoveUndoUnit& undo)
{
    MoveExecutor executor(m_position.edit());
    executor.makeMove<validation>(move, undo, m_gameState.halfmoveClock);

    // back at white's turn means black just moved, which completes a full move.
    m_gameState.whiteToMove = !m_gameState.whiteToMove;
    m_gameState.moveCount += static_cast<u16>(m_gameState.whiteToMove);
}

template void Chessboard::makeMove<false>(const PackedMove move, MoveUndoUnit& undo);
template void Chessboard::makeMove<true>(const PackedMove move, MoveUndoUnit& undo);

bool Chessboard::unmakeMove(const MoveUndoUnit& undo)
{
    MoveExecutor executor(m_position.edit());
    const bool result = executor.unmakeMove(undo);

    m_gameState.moveCount -= static_cast<u16>(m_gameState.whiteToMove);
    m_gameState.whiteToMove = !m_gameState.whiteToMove;
    m_gameState.halfmoveClock = undo.halfmoveClock;
    return result;
}

void Chessboard::makeNullMove()
{
    PositionEditor position = m_position.edit();
    position.enPassant().clear();
    position.hash() = zobrist::updateBlackToMoveHash(position.hash());
    ++m_gameState.halfmoveClock;
    m_gameState.whiteToMove = !m_gameState.whiteToMove;
    m_gameState.moveCount += static_cast<u16>(m_gameState.whiteToMove);
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
            ranks[boardItr.rank()] << "\n" << (boardItr.rank() + 1) << "  ";
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