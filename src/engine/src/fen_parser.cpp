#include "fen_parser.h"
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <list>

#include "defines.h"
#include "chessboard.h"
#include "game_context.h"

bool deserializeCastling(const std::string& castlingStr, GameContext& outputContext)
{
    byte castlingState = 0x00;
    if (castlingStr[0] != '-')
    {
        for (char cstling : castlingStr)
        {
            switch (cstling)
            {
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

    outputContext.editChessboard().editCastlingState() = castlingState;
    return true;
}

bool deserializeBoard(const std::string& boardStr, GameContext& outputContext)
{
    LOG_INFO() << boardStr.c_str();
    std::istringstream ssboard(boardStr);
    std::list<std::string> ranks;
    std::string rank;
    while (std::getline(ssboard, rank, '/'))
    {
        ranks.push_back(rank);
    }

    if (ranks.size() != 8)
        return false;

    auto boardItr = outputContext.editChessboard().begin();
    while (!ranks.empty())
    {        
        const char* rdr = ranks.back().c_str();
        while (*rdr != '\0')
        {
            char value = *rdr;

            if (std::isdigit(value))
            {
                signed short steps = value - '0';
                LOG_ERROR_EXPR(steps > 0) << "Steps can't be less than zero and should never be in this situation.";
                boardItr += steps;
            }
            else if (value == '/')
            {
                ++boardItr;
            }
            else
            {
                if (!(*boardItr).editPiece().fromString(value))
                    return false;
                ++boardItr;
            }

            ++rdr;
        }

        ranks.pop_back();
    }

    return true;
}

bool deserializeToPlay(const std::string& toPlayStr, GameContext& outputContext)
{
    char value = std::tolower(toPlayStr[0]);
    if (value == 'w')
        outputContext.editToPlay() = PieceSet::WHITE;
    else if (value == 'b')
        outputContext.editToPlay() = PieceSet::BLACK;
    else
        return false;

    return true;
}

bool deserializeEnPassant(const std::string& enPassantStr, GameContext& outputContext)
{
    outputContext.editChessboard().editEnPassant() = Notation();
    if (enPassantStr.size() > 1)
    {
        byte file = enPassantStr[0];
        byte rank = (byte)std::atoi(&enPassantStr[1]);
        outputContext.editChessboard().editEnPassant() = Notation::BuildPosition(file, rank);
    }
    return true;
}

bool FENParser::deserialize(const char* input, GameContext& outputContext)
{
    std::istringstream ssfen(input);
    std::list<std::string> tokens;
    std::string token;
    while (std::getline(ssfen, token, ' '))
    {
        tokens.push_back(token);
    }

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

    outputContext.editPly() = plyCount;
    outputContext.editMoveCount() = moveCount;

    if (!tokens.empty())
        return false;

    return true;
}