#include <core/game.hpp>

#include <elephant_gambit.hpp>
#include <io/fen_parser.hpp>

#include <optional>

Game::Game()
{
    chess::NewGame(m_board);
}

bool Game::loadFen(std::string_view fen)
{
    const std::optional<Chessboard> board = io::fen_parser::read(fen);
    if (!board)
        return false;
    *this = Game(*board);
    return true;
}

template<bool validation>
void Game::makeMove(PackedMove move)
{
    m_board.makeMove<validation>(move, m_moves.emplace_back());
}

template void Game::makeMove<false>(PackedMove move);
template void Game::makeMove<true>(PackedMove move);

bool Game::unmakeMove()
{
    if (m_moves.empty())
        return false;

    const bool result = m_board.unmakeMove(m_moves.back());
    m_moves.pop_back();
    return result;
}
