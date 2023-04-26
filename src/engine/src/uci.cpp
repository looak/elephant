#include "uci.hpp"

#include "fen_parser.h"
#include "game_context.h"
#include "move.h"

#include <map>
#include <functional>
#include <optional>


UCI::UCI() :
    m_stream(std::cout),
    m_enabled(true)
{
    m_stream << "id name Elephant Gambit\n";
    m_stream << "id author Alexander Loodin Ek\n";
}

UCI::~UCI()
{
    m_stream << "quit\n";
}

bool UCI::Enabled()
{    
    if (m_enabled)
        m_stream << "uciok\n";
    return m_enabled;
}

void UCI::Disable()
{
    m_enabled = false;
}

bool UCI::IsReady()
{
    m_stream << "readyok\n";
    return true;
}

bool UCI::Position(std::list<std::string>& args)
{
    if (args.size() == 0)
    {
        LOG_ERROR() << "PositionCommand: No arguments";
        return false;
    }

    auto&& arg = args.front();
    
    if (arg == "startpos")
    {
        args.pop_front();
        m_context.NewGame();
    }
    else if (arg == "fen")
    {     
        args.pop_front();
        std::string fen = "";
        while (args.size() > 0 && args.front() != "moves")
        {
             fen += args.front() + " ";
             args.pop_front();
        }

        if (!FENParser::deserialize(fen.c_str(), m_context))
        {
            LOG_ERROR() << "Failed to parse fen: " << fen;
            return false;
        }
    }

    if (args.size() == 0)
        return true;

    if (args.front() == "moves")
    {            
        args.pop_front();
        while (args.size() > 0)
        {
            std::string moveStr = args.front();
            args.pop_front();
            Move move = Move::fromString(moveStr);
            if (!m_context.MakeMove(move))
            {
                LOG_ERROR() << "Failed to make move: " << move.toString();
                return false;
            }
            LOG_DEBUG() << "Made move: " << move.toString();
        }
    }

    return true;
}

bool UCI::NewGame()
{
    m_context.NewGame();
    m_stream << "isready\n";
    return true;
}

bool UCI::Go(std::list<std::string>& args)
{
    // some of these args have values associated with them, so when we iterate
    // over the options, some times we need to jump twice. Hence the lambda
    // returns a optional, since the lambda can also fail.
    std::map<std::string, std::function<std::optional<int>(void)>> options;
    options["searchmoves"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["ponder"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["wtime"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["btime"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["winc"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["binc"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["movestogo"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["nodes"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["mate"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["movetime"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["infinite"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };

    SearchParamters params;
    options["depth"] = [&params, this, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "depth");
        itr++; // increment itr should hold the value of "depth"
        if (itr == args.end())
        {
            LOG_ERROR() << "No depth specified";
            return std::nullopt;
        }
        params.SearchDepth = std::stoi(*itr);
        m_stream << "info depth " << params.SearchDepth << "\n";
        return 1;
    };

    for (auto argItr = args.begin(); argItr != args.end(); ++argItr)
    {
        auto itr = options.find(*argItr);
        if (itr == options.end())
        {
            LOG_ERROR() << "Unknown option: " << *argItr;
            return false;
        }
        std::optional<int> optinalResult = itr->second();
        if (optinalResult)
        {
            int increments = *optinalResult;
            argItr = std::next(argItr, increments);
            if (argItr == args.end())
                break;
        }
        else
        {
            LOG_ERROR() << "Invalid option: " << *argItr;
            return false;
        }
    }

    m_context.CalculateBestMove(params);
    return true;
}