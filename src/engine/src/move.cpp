#include "move.h"

#include <list>
#include <sstream>

Move::Move(Notation source, Notation target) :
    TargetSquare(target),
    SourceSquare(source),
    EnPassantTargetSquare(InvalidNotation),
    PrevCastlingState(0),
    Piece(ChessPiece()),
    PromoteToPiece(ChessPiece()),
    CapturedPiece(ChessPiece()),
    Flags(MoveFlag::Zero),
    Score(0)
{
}

Move::Move() :
    TargetSquare(Notation::Invalid()),
    SourceSquare(Notation::Invalid()),
    EnPassantTargetSquare(InvalidNotation),
    PrevCastlingState(0),
    Piece(ChessPiece()),
    PromoteToPiece(ChessPiece()),
    CapturedPiece(ChessPiece()),
    Flags(MoveFlag::Invalid),
    Score(0),
    PrevMove(nullptr),
    NextMoveCount(0),
    NextMove(nullptr)
{
}

Move::Move(const Move& other) { *this = other; }

Move&
Move::operator=(const Move& other)
{
    TargetSquare = Notation(other.TargetSquare);
    SourceSquare = Notation(other.SourceSquare);
    Piece = other.Piece;
    PromoteToPiece = other.PromoteToPiece;
    Flags = other.Flags;
    PrevMove = other.PrevMove;
    NextMoveCount = other.NextMoveCount;
    NextMove = other.NextMove;
    PrevCastlingState = other.PrevCastlingState;
    CapturedPiece = other.CapturedPiece;
    EnPassantTargetSquare = other.EnPassantTargetSquare;
    Score = other.Score;

    return *this;
}

Move::Move(Move&& other) { *this = std::move(other); }

PackedMove
Move::readPackedMove() const
{
    u16 packedMove = 0;
    packedMove = (SourceSquare.index() & 63);
    packedMove |= (TargetSquare.index() & 63) << 6;
    packedMove |= isCapture() << 14;
    if (isCastling()) {
        packedMove |= 1 << 13;
        if (TargetSquare.file == file_c)  // queen side castling
            packedMove |= 1 << 12;
    }
    if (isPromotion()) {
        packedMove |= 1 << 15;

        u8 packedPiece = PromoteToPiece.type() - 2;
        packedMove |= packedPiece << 12;
    }

    if (Piece.isPawn()) {
        if (abs(SourceSquare.rank - TargetSquare.rank) == 2)
            packedMove |= 1 << 12;
    }

    PackedMove mv;
    mv.set(packedMove);
    return mv;
}

i16
Move::calcCaptureValue() const
{
    i16 victim = ChessPieceDef::Value(CapturedPiece.index());
    i16 attacker = ChessPieceDef::Value(Piece.index());

    return victim * 10 - attacker;
}

void
ParsePiece(const std::string& moveStr, size_t& cursor, Move& mv, bool isWhite)
{
    char character = moveStr.at(cursor);
    if (character == 'O')  // castling move
    {
        // we will either have two or three O's in a row, seperated by dashes.
        int o_counter = 1;
        char lookingFor = '-';
        while (true) {
            cursor++;
            if (cursor >= moveStr.length()) {
                break;
            }

            character = moveStr.at(cursor);
            if (character == lookingFor) {
                if (character == 'O') {
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
        byte rank = isWhite ? rank_1 : rank_8;
        byte file = o_counter == 3 ? file_c : file_g;  // o_counter == 3 means queen side castling
        mv.TargetSquare = Notation(file, rank);
    }
    else {
        if (std::isupper(character)) {
            if (isWhite == false)
                character = std::tolower(character);

            mv.Piece.fromString(character);
            cursor++;
        }
        else {
            mv.Piece = ChessPiece(isWhite ? Set::WHITE : Set::BLACK, PieceType::PAWN);
        }
    }
}

Notation
ReadWholeNotation(const std::string& moveStr, size_t& cursor)
{
    byte file = moveStr.at(cursor);
    byte rank = moveStr.at(cursor + 1);

    file = file - 'a';
    rank = rank - '1';
    return Notation(file, rank);
}

void
ParseFileAndRank(const std::string& moveStr, size_t& cursor, Move& mv, bool)
{
    if (cursor >= moveStr.length())
        return;

    // read file & rank
    char character = moveStr.at(cursor);

    // character might be a captial O in case castling cases
    if (isupper(character))
        return;

    if (character == 'x')  // we're looking at a capture
    {
        mv.Flags |= MoveFlag::Capture;
        cursor++;
        character = moveStr.at(cursor);
    }

    bool readPosition = false;
    // when moves need more information to distinguish them, there might be additional characters
    // at this portion, examples; 1) Rdf8, 2) R1a3, 3) Qh4e1
    if (isdigit(character)) {
        byte rank = character - '1';
        mv.SourceSquare = Notation(9, rank);
        cursor++;
    }
    else {
        // verify if next two are a whole notation or only file.
        byte scnd = moveStr.at(cursor + 1);
        if (isdigit(scnd)) {
            mv.TargetSquare = ReadWholeNotation(moveStr, cursor);
            cursor++;
            readPosition = true;
        }
        else {
            byte file = character - 'a';
            mv.SourceSquare = Notation(file, 9);
            // we're a pawn capture if we have two files in a row and our piece is a pwan.
            // since pawns can only move between files when capturing
            if (mv.Piece.getType() == PieceType::PAWN)
                mv.Flags |= MoveFlag::Capture;
        }
        cursor++;
    }

    if (cursor >= moveStr.length())
        return;

    character = moveStr.at(cursor);
    // there might be a capture indicator here too
    if (character == 'x')  // we're looking at a capture
    {
        mv.Flags |= MoveFlag::Capture;
        cursor++;
        character = moveStr.at(cursor);
    }
    if (islower(character)) {
        auto target = ReadWholeNotation(moveStr, cursor);
        cursor += 2;

        if (readPosition) {
            mv.SourceSquare = mv.TargetSquare;
            mv.setAmbiguous(false);
        }
        mv.TargetSquare = target;
    }

    // read any checks or checkmate indications
    if (cursor < moveStr.length()) {
        byte character = moveStr.at(cursor);
        if (character == '+') {
            mv.Flags |= MoveFlag::Check;
            cursor++;
        }
        else if (character == '#') {
            mv.Flags |= MoveFlag::Checkmate;
            cursor++;
        }
    }
}

std::string
trim(const std::string& str, const std::string& whitespace = " \t")
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return "";  // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

std::vector<std::string>
Move::ParsePGN(std::string pgn, std::vector<Move>& ret)
{
    std::vector<std::string> comments;
    std::list<std::string> tokens;
    std::string token;
    size_t cursor = pgn.find('{');
    size_t endPos = 0;

    while (cursor != std::string::npos) {
        endPos = pgn.find('}');
        endPos++;  // inclusive
        token = pgn.substr(cursor, endPos - cursor);
        comments.push_back(token);
        pgn.erase(cursor, endPos - cursor);
        cursor = pgn.find('{');
    }

    cursor = 0;
    endPos = pgn.find('.', 2);

    while (endPos != std::string::npos) {
        size_t orgEnd = endPos + 1;
        endPos--;  // step back one.
        while (std::isdigit(pgn.at(endPos)))
            endPos--;

        token = pgn.substr(cursor, endPos - cursor);
        tokens.push_back(trim(token));
        cursor = endPos;
        endPos = pgn.find('.', orgEnd);
    }

    if (endPos == std::string::npos) {
        token = pgn.substr(cursor, pgn.size() - cursor);
        tokens.push_back(trim(token));
    }

    for (auto moveStr : tokens) {
        // split token
        std::istringstream ssboard(moveStr);
        std::vector<std::string> notations;
        std::string notation;
        while (std::getline(ssboard, notation, ' ')) {
            notation.erase(std::find(notation.begin(), notation.end(), ' '), notation.end());
            notations.push_back(notation);
        }

        auto& whiteMv = ret.emplace_back();
        whiteMv.setAmbiguous(true);
        bool isWhite = true;

        // white move
        size_t cursor = 0;
        ParsePiece(notations[1], cursor, whiteMv, isWhite);
        ParseFileAndRank(notations[1], cursor, whiteMv, isWhite);

        if (notations.size() > 2) {
            auto& blackMv = ret.emplace_back();
            blackMv.setAmbiguous(true);
            isWhite = false;
            cursor = 0;
            ParsePiece(notations[2], cursor, blackMv, isWhite);
            ParseFileAndRank(notations[2], cursor, blackMv, isWhite);
        }
    }

    // fixup pointers
    size_t index = 0;
    for (auto&& mv : ret) {
        if (index > 0)
            mv.PrevMove = &ret[index - 1];

        if (index < ret.size() - 1)
            mv.NextMove = &ret[index + 1];

        index++;
    }

    return comments;
}

Move
Move::fromPGN(std::string pgn, bool isWhiteMove)
{
    Move mv;
    mv.setAmbiguous(true);
    mv.SourceSquare = Notation::Invalid();
    mv.TargetSquare = Notation::Invalid();

    size_t cursor = 0;
    ParsePiece(pgn, cursor, mv, isWhiteMove);
    ParseFileAndRank(pgn, cursor, mv, isWhiteMove);
    return mv;
}

std::string
Move::toString() const
{
    std::string ret;
    ret += SourceSquare.toString();
    ret += TargetSquare.toString();

    if (isPromotion()) {
        ret += PromoteToPiece.toString();
    }

    return ret;
}

Move
Move::fromString(std::string str)
{
    Move mv;
    mv.setAmbiguous(false);
    mv.SourceSquare = Notation::Invalid();
    mv.TargetSquare = Notation::Invalid();

    mv.SourceSquare = Notation::BuildPosition(str[0], std::atoi(&str[1]));
    mv.TargetSquare = Notation::BuildPosition(str[2], std::atoi(&str[3]));

    if (str.size() == 4) {
        if (str[4] == '=' || str[4] == '#')
            return mv;
        mv.PromoteToPiece.fromString(str[4]);
        mv.Flags |= MoveFlag::Promotion;
    }
    else if (str.size() == 5) {
        mv.PromoteToPiece.fromString(str[4]);
        mv.Flags |= MoveFlag::Promotion;
    }

    return mv;
}