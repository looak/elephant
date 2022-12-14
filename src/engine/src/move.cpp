#include "move.h"
#include <list>

Move::Move(const Notation& source, const Notation& target) :
    TargetSquare(target),
    SourceSquare(source),
    Piece(ChessPiece()),
    Promote(ChessPiece()),
    Flags(MoveFlag::Zero),
    PrevMove(nullptr),
    NextMoveCount(0),
    NextMove(nullptr)
{
}

Move::Move() :
    TargetSquare(0),
    SourceSquare(0),
    Piece(ChessPiece()),
    Promote(ChessPiece()),
    Flags(MoveFlag::Zero),
    PrevMove(nullptr),
    NextMoveCount(0),
    NextMove(nullptr)
{
}

Move::Move(const Move& other)
{
    *this = other;
}

Move& Move::operator=(const Move& other)
{   
    TargetSquare = Notation(other.TargetSquare);
    SourceSquare = Notation(other.SourceSquare);
    Piece = other.Piece;
    Promote = other.Promote;
    Flags = other.Flags;
    PrevMove = other.PrevMove;
    NextMoveCount = other.NextMoveCount;
    NextMove = other.NextMove;

    return *this;
}

void ParsePiece(const std::string& moveStr, size_t& cursor, Move& mv, bool isWhite)
{
    char character = moveStr.at(cursor);
    if (character == 'O') // castling move
    {
        // we will either have two or three O's in a row, seperated by dashes.
        int o_counter = 1;
        char lookingFor = '-';
        while (true)
        {
            cursor++;
            if (cursor >= moveStr.length())
            {
                break;
            }

            character = moveStr.at(cursor);
            if (character == lookingFor)
            {
                if (character == 'O')
                {
                    lookingFor = '-';
                    o_counter++;
                }
                else
                    lookingFor = 'O';
            }
            else
                break;
        }

        mv.Piece = ChessPiece(isWhite ? Set::WHITE : Set::BLACK, PieceType::KING);
        mv.Flags |= MoveFlag::Castle;
        byte rank = isWhite ? 0 : 7;
        byte file = o_counter == 3 ? 0 : 7; // o_counter == 3 means queen side castling
        mv.TargetSquare = Notation(file, rank);
    }
    else
    {
        if (std::isupper(character))
        {
            if (isWhite == false)
                character = std::tolower(character);

            mv.Piece.fromString(character);
            cursor++;
        }
        else
        {
            mv.Piece = ChessPiece(isWhite ? Set::WHITE : Set::BLACK, PieceType::PAWN);
        }
    }
}

void ParseFileAndRank(const std::string& moveStr, size_t& cursor, Move& mv, bool isWhite)
{
    if (cursor >= moveStr.length())
        return;

    // read file & rank
    char character = moveStr.at(cursor);

    // character might be a captial O in case castling cases
    if (isupper(character))
        return;

    if (character == 'x') // we're looking at a capture
    {
        mv.Flags |= MoveFlag::Capture;
        cursor++;
        character = moveStr.at(cursor);
    }

    byte srcFile = 0;
    byte file = character - 'a';
    cursor++;
    character = moveStr.at(cursor);

    // two lower case non-digit characters in a row are a indication of a pawn capture.
    // i.e. 1. de4 -> Pawn move from d3 to e4.(or d5 to e4 in case we're a black pawn).
    bool isPawnCapture = std::isdigit(character) == false;
    if (isPawnCapture && mv.Piece.getType() == PieceType::PAWN)
    {
        srcFile = file;
        file = character - 'a';
        mv.Flags |= MoveFlag::Capture;
        cursor++;

        character = moveStr.at(cursor);
    }
    // preferably I wouldn't need this check again, but since the pawn code will move the cursor
    // forward, I need to verify that it's a digit.
    if (std::isdigit(character) == false)
    {
        LOG_ERROR() << "Parsing PNG is not a digit?";
    }
                    
    byte rank = character - '1';

    mv.TargetSquare = Notation(file, rank);
    if (isPawnCapture) // we're only building a source square if we're a pawn capture.
        mv.SourceSquare = Notation(srcFile, rank + (isWhite ? -1 : 1));

    cursor++;

    // read any checks or checkmate indications
    if (cursor < moveStr.length())
    {
        byte character = moveStr.at(cursor);
        if (character == '+')
        {
            mv.Flags |= MoveFlag::Check;
            cursor++;
        }
        else if (character == '#')
        {
            mv.Flags |= MoveFlag::Checkmate;
            cursor++;
        }
    }
}

std::vector<std::string> 
Move::ParsePNG(std::string png, std::vector<Move>& ret)
{       
    std::vector<std::string> comments;
    std::list<std::string> tokens;
    std::string token;
    size_t cursor = png.find('{');
    size_t endPos = 0;

    while (cursor != std::string::npos)
    {
        endPos = png.find('}');
        endPos++; // inclusive
        token = png.substr(cursor, endPos - cursor);
        comments.push_back(token);
        png.erase(cursor, endPos - cursor);
        cursor = png.find('{');
    }

    cursor = 0;
    endPos = png.find('.', 2);
    
    while (endPos != std::string::npos)
    {
        size_t orgEnd = endPos+1;
        endPos--; // step back one.
        while (std::isdigit(png.at(endPos)))
            endPos--;

        token = png.substr(cursor, endPos - cursor);
        token.erase(remove(token.begin(), token.end(), ' '), token.end());
        tokens.push_back(token);
        cursor = endPos;
        endPos = png.find('.', orgEnd);
    }

    if (endPos == std::string::npos)
    {
        token = png.substr(cursor, png.size() - cursor);
        token.erase(remove(token.begin(), token.end(), ' '), token.end());
        tokens.push_back(token);
    }

    for (auto moveStr : tokens)
    {
        auto& whiteMv = ret.emplace_back();
        bool isWhite = true;
        cursor = moveStr.find('.');
        cursor++;

        ParsePiece(moveStr, cursor, whiteMv, isWhite);
        ParseFileAndRank(moveStr, cursor, whiteMv, isWhite);

        auto& blackMv = ret.emplace_back();
        isWhite = false;
        ParsePiece(moveStr, cursor, blackMv, isWhite);
        ParseFileAndRank(moveStr, cursor, blackMv, isWhite);
    }

    // fixup pointers
    int index = 0;
    for (auto&& mv : ret)
    {
        if (index > 0)
            mv.PrevMove = &ret[index - 1];

        if (index < ret.size() - 1)
            mv.NextMove = &ret[index + 1];

        index++;
    }

    return comments;
}