#include "fen_parser.h"
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <list>
#include <sstream>
#include <string>

#include "chessboard.h"
#include "defines.hpp"
#include "game_context.h"

bool
deserializeCastling(const std::string& castlingStr, GameContext& outputContext)
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

    outputContext.editChessboard().setCastlingState(castlingState);
    return true;
}

bool
deserializeBoard(const std::string& boardStr, GameContext& outputContext)
{
    std::istringstream ssboard(boardStr);
    std::list<std::string> ranks;
    std::string rank;
    while (std::getline(ssboard, rank, '/')) {
        ranks.push_back(rank);
    }

    if (ranks.size() != 8)
        return false;

    auto& board = outputContext.editChessboard();
    auto boardItr = board.begin();
    while (!ranks.empty()) {
        const char* rdr = ranks.back().c_str();
        while (*rdr != '\0') {
            char nullterminated_value[2];
            nullterminated_value[0] = *rdr;
            nullterminated_value[1] = '\0';

            char value = nullterminated_value[0];

            if (std::isdigit(value)) {
                signed short steps = 0;
                std::from_chars(&nullterminated_value[0], &nullterminated_value[1], steps);
                LOG_ERROR_EXPR(steps > 0) << "Steps can't be less than zero and should never be in this situation.";
                boardItr += steps;
            }
            else if (value == '/') {
                ++boardItr;
            }
            else {
                ChessPiece piece;
                if (!piece.fromString(value))
                    return false;

                board.PlacePiece(piece, boardItr.square());
                ++boardItr;
            }

            ++rdr;
        }

        ranks.pop_back();
    }

    return true;
}

bool
deserializeToPlay(const std::string& toPlayStr, GameContext& outputContext)
{
    char value = std::tolower(toPlayStr[0]);
    if (value == 'w')
        outputContext.editChessboard().setToPlay(Set::WHITE);
    else if (value == 'b')
        outputContext.editChessboard().setToPlay(Set::BLACK);
    else
        return false;

    return true;
}

bool
deserializeEnPassant(const std::string& enPassantStr, GameContext& outputContext)
{
    outputContext.editChessboard().setEnPassant(Notation());
    if (enPassantStr.size() > 1) {
        byte file = enPassantStr[0];
        byte rank = (byte)std::atoi(&enPassantStr[1]);
        outputContext.editChessboard().setEnPassant(Notation::BuildPosition(file, rank));
    }
    return true;
}

bool
FENParser::deserialize(const char* input, GameContext& outputContext)
{
    std::istringstream ssfen(input);
    std::list<std::string> tokens;
    std::string token;
    while (std::getline(ssfen, token, ' ')) {
        tokens.push_back(token);
    }

    outputContext.Reset();

    if (tokens.size() != 6)
        return false;

    if (!deserializeBoard(tokens.front(), outputContext))
        return false;

    tokens.pop_front();

    if (!deserializeToPlay(tokens.front(), outputContext))
        return false;

    tokens.pop_front();

    if (!deserializeCastling(tokens.front(), outputContext))
        return false;

    tokens.pop_front();

    if (!deserializeEnPassant(tokens.front(), outputContext))
        return false;

    tokens.pop_front();

    byte plyCount = std::atoi(&tokens.front()[0]);
    tokens.pop_front();

    byte moveCount = std::atoi(&tokens.front()[0]);
    tokens.pop_front();

    outputContext.editChessboard().setPlyAndMoveCount(plyCount, moveCount);

    if (!tokens.empty())
        return false;

    return true;
}

bool
FENParser::serialize(const GameContext& inputContext, std::string& resultFen)
{
    const auto& board = inputContext.readChessboard();

    serialize(board, inputContext.readToPlay(), resultFen);

    resultFen += ' ';

    resultFen += std::to_string(inputContext.readPly()) + " ";
    resultFen += std::to_string(inputContext.readMoveCount());

    return true;
}

bool
FENParser::serialize(const Chessboard& board, Set toPlay, std::string& resultFen)
{
    auto itr = board.begin();

    std::vector<std::string> ranks;
    byte rank = itr.rank();
    std::string strngBuilder;
    int emptyFiles = 0;
    while (itr != board.end()) {
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
    if (toPlay == Set::WHITE)
        resultFen += " w";
    else
        resultFen += " b";

    resultFen += ' ';

    auto castlingState = board.readCastlingState();
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

    if (board.readPosition().readEnPassant())
        resultFen += board.readPosition().readEnPassant().toString();
    else
        resultFen += '-';

    return true;
}