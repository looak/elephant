#include <io/fen_parser.hpp>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <list>
#include <sstream>
#include <string>

#include <elephant_gambit.hpp>
#include <core/chessboard.hpp>
#include <defines.hpp>
#include <core/game_context.hpp>
#include <position/castling_state_info.hpp>

namespace io {
namespace fen_parser {
namespace internals {
std::list<std::string_view> tokenize(std::string_view s, char delim) {
    std::list<std::string_view> tokens;

    while (true) {
        auto pos = s.find(delim);
        if (pos == std::string_view::npos) {
            if (!s.empty())
                tokens.push_back(s);
            break;
        }
        tokens.push_back(s.substr(0, pos));
        s.remove_prefix(pos + 1);
    }

    return tokens;
}
} // namespace internals

bool
deserializeCastling(std::string_view castlingStr, PositionEditor position)
{
    byte castlingState = 0x00;
    if (castlingStr[0] != '-') {
        for (char cstling : castlingStr) {
            switch (cstling) {
                case 'k':
                    castlingState |= 0x04;
                    break;
                case 'q':
                    castlingState |= 0x08;
                    break;
                case 'K':
                    castlingState |= 0x01;
                    break;
                case 'Q':
                    castlingState |= 0x02;
                    break;
                default:
                    return false;
            }
        }
    }

    position.castling().write(castlingState);
    return true;
}

bool deserializeBoard(std::string_view fen, PositionEditor position)
{
    std::list<std::string_view> ranks = internals::tokenize(fen, '/');

    // Handle case where the last rank may contain extra info (to move, castling, etc.)
    std::list<std::string_view> lastRank = internals::tokenize(ranks.back(), ' ');
    if (lastRank.size() > 1) {
        ranks.pop_back();
        ranks.push_back(lastRank.front());
    }

    if (ranks.size() != 8)
        return false;

    auto boardWriter = position.begin();
    while (!ranks.empty()) {
        auto fileReader = ranks.back().begin();        
        while (fileReader != ranks.back().end()) {

            if (std::isdigit(*fileReader)) {
                signed short steps = 0;
                std::from_chars(&(*fileReader), &(*fileReader) + 1, steps);
                THROW_EXPR(steps > 0, ephant::io_error, "Steps can't be less than zero and should never be in this situation.");
                boardWriter += steps;                
            }
            else if (*fileReader == '/') {
                ++boardWriter;

            }
            else {
                ChessPiece piece;
                if (!piece.fromString(*fileReader))
                    return false;

                position.placePiece(piece, boardWriter.square());
                ++boardWriter;
            }

            ++fileReader;
        }

        ranks.pop_back();
    }

    return true;
}

bool
deserializeToPlay(std::string_view toPlayStr, Chessboard& outputBoard)
{
    char value = std::tolower(toPlayStr[0]);
    if (value == 'w')
        outputBoard.editState().whiteToMove = true;
    else if (value == 'b')
        outputBoard.editState().whiteToMove = false;
    else
        return false;

    return true;
}

bool
deserializeEnPassant(std::string_view enPassantStr, PositionEditor position)
{
    position.enPassant().clear();
    if (enPassantStr.size() > 1) {
        byte file = enPassantStr[0] - 'a';
        byte rank = (byte)std::atoi(&enPassantStr[1]) - 1;
        position.enPassant().writeSquare(toSquare(file, rank));
    }
    return true;
}

bool deserialize(std::string_view input, PositionEditor outPosition)
{
    return deserializeBoard(input, outPosition);
}

bool deserialize(std::string_view input, Chessboard& outputBoard)
{
    std::list<std::string_view> tokens = internals::tokenize(input, ' ');

    chess::ClearBoard(outputBoard);

    if (tokens.size() < 4 || tokens.size() > 6)
        return false;

    bool containsMoveCounts = false;
    if (tokens.size() == 6)
        containsMoveCounts = true;   

    if (!deserializeBoard(tokens.front(), outputBoard.editPosition()))
        return false;

    tokens.pop_front();

    if (!deserializeToPlay(tokens.front(), outputBoard))
        return false;

    tokens.pop_front();

    if (!deserializeCastling(tokens.front(), outputBoard.editPosition()))
        return false;

    tokens.pop_front();

    if (!deserializeEnPassant(tokens.front(), outputBoard.editPosition()))
        return false;

    tokens.pop_front();

    if (containsMoveCounts == true) {
        byte plyCount = std::atoi(&tokens.front()[0]);
        tokens.pop_front();

        byte moveCount = std::atoi(&tokens.front()[0]);
        tokens.pop_front();

        outputBoard.editState().plyCount = plyCount;
        outputBoard.editState().moveCount = moveCount;
    }

    if (!tokens.empty())
        return false;

    return true;
}

bool deserialize(const char* input, Chessboard& outputBoard)
{
    std::istringstream ssfen(input);
    std::list<std::string> tokens;
    std::string token;
    while (std::getline(ssfen, token, ' ')) {
        tokens.push_back(token);
    }

    chess::ClearBoard(outputBoard);

    if (tokens.size() < 4 || tokens.size() > 6)
        return false;

    bool containsMoveCounts = false;
    if (token.size() == 6)
        containsMoveCounts = true;    

    if (!deserializeBoard(tokens.front(), outputBoard.editPosition()))
        return false;

    tokens.pop_front();

    if (!deserializeToPlay(tokens.front(), outputBoard))
        return false;

    tokens.pop_front();

    if (!deserializeCastling(tokens.front(), outputBoard.editPosition()))
        return false;

    tokens.pop_front();

    if (!deserializeEnPassant(tokens.front(), outputBoard.editPosition()))
        return false;

    tokens.pop_front();

    if (containsMoveCounts == true) {
        byte plyCount = std::atoi(&tokens.front()[0]);
        tokens.pop_front();

        byte moveCount = std::atoi(&tokens.front()[0]);
        tokens.pop_front();

        outputBoard.editState().plyCount = plyCount;
        outputBoard.editState().moveCount = moveCount;
    }

    if (!tokens.empty())
        return false;

    return true;
}

bool serialize(const Chessboard& board, std::string& resultFen)
{
    auto position = board.readPosition();
    auto itr = position.begin();

    std::vector<std::string> ranks;
    byte rank = itr.rank();
    std::string strngBuilder;
    int emptyFiles = 0;
    while (itr != position.end()) {
        ChessPiece cp = itr.get();
        if (cp.isValid()) {
            if (emptyFiles > 0) {
                strngBuilder += std::to_string(emptyFiles);
                emptyFiles = 0;
            }

            strngBuilder += cp.toString();
        }
        else {
            emptyFiles++;
        }

        itr++;

        if (rank != itr.rank()) {
            if (emptyFiles > 0) {
                strngBuilder += std::to_string(emptyFiles);
                emptyFiles = 0;
            }

            rank = itr.rank();
            ranks.push_back(strngBuilder);
            strngBuilder.clear();
        }
    }

    auto rankItr = ranks.rbegin();
    strngBuilder.clear();
    while (true) {
        strngBuilder += (*rankItr);
        rankItr++;

        if (rankItr != ranks.rend())
            strngBuilder += '/';
        else
            break;
    }

    resultFen = strngBuilder;

    // set to move
    if (board.readState().whiteToMove)
        resultFen += " w";
    else
        resultFen += " b";

    resultFen += ' ';

    auto castlingState = position.castling();
    byte rawCastlingState = castlingState.read();
    if (rawCastlingState > 0) {
        if (rawCastlingState & 1)
            resultFen += "K";
        if (rawCastlingState & 2)
            resultFen += "Q";
        if (rawCastlingState & 4)
            resultFen += "k";
        if (rawCastlingState & 8)
            resultFen += "q";
    }
    else {
        resultFen += '-';
    }

    resultFen += ' ';

    if (position.enPassant())
        resultFen += position.enPassant().toString();
    else
        resultFen += '-';


    
    resultFen += ' ';

    resultFen += std::to_string(board.readState().plyCount) + " ";
    resultFen += std::to_string(board.readState().moveCount);

    return true;
}

} // namespace fen_parser
} // namespace io