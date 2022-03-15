#include "fen_parser.h"
#include "defines.h"
#include <cctype>
#include <cstdlib>

bool FENParser::deserialize(const char* input, GameContext& outputContext)
{
    byte boardIndx = 0;
    const char* rdr = input;
    while (*rdr != '\0')
    {
        char value = *rdr;
        
        if (std::isdigit(value))
        {
            byte steps = (byte)std::atoi(&value);
            boardIndx += steps;
        }
        else if (value == '/')
        {
            boardIndx++;
        }
        rdr++;
    }

    return false;
}