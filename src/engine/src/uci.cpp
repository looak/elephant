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
    return m_enabled;
}

void UCI::Enable()
{
    m_enabled = true;
    m_stream << "uciok\n";
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
        }
    }

    return true;
}

bool UCI::NewGame()
{
    m_context.NewGame();
    return true;
}

bool UCI::Stop()
{
    return true;
}

bool UCI::Go(std::list<std::string>& args)
{
    SearchParameters searchParams;
    EngineParameters engineParams;
    
    // some of these args have values associated with them, so when we iterate
    // over the options, some times we need to jump twice. Hence the lambda
    // returns a optional, since the lambda can also fail.
    std::map<std::string, std::function<std::optional<int>(void)>> options;
    options["searchmoves"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["ponder"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["wtime"] = [&engineParams, this, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "wtime");
        itr++; // increment itr should hold the value of "movetime"
        if (itr == args.end())
        {
            LOG_ERROR() << "No time specified";
            return std::nullopt;
        }
        engineParams.WhiteTimelimit = std::stoi(*itr);
        m_stream << "info wtime " << engineParams.WhiteTimelimit << "\n";
        return 1;
    };
    options["btime"] = [&engineParams, this, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "btime");
        itr++; // increment itr should hold the value of "movetime"
        if (itr == args.end())
        {
            LOG_ERROR() << "No time specified";
            return std::nullopt;
        }
        engineParams.BlackTimelimit = std::stoi(*itr);
        m_stream << "info btime " << engineParams.BlackTimelimit << "\n";
        return 1;
    };
    options["winc"] = [&engineParams, this, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "winc");
        itr++; // increment itr should hold the value of "movetime"
        if (itr == args.end())
        {
            LOG_ERROR() << "No time specified";
            return std::nullopt;
        }
        engineParams.WhiteTimeIncrement = std::stoi(*itr);
        m_stream << "info winc " << engineParams.WhiteTimeIncrement << "\n";
        return 1;
    };
    options["binc"] = [&engineParams, this, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "binc");
        itr++; // increment itr should hold the value of "movetime"
        if (itr == args.end())
        {
            LOG_ERROR() << "No time specified";
            return std::nullopt;
        }
        engineParams.BlackTimeIncrement = std::stoi(*itr);
        m_stream << "info binc " << engineParams.BlackTimeIncrement << "\n";
        return 1;
    };
    options["movestogo"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["nodes"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["mate"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };
    options["movetime"] = [&searchParams, this, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "movetime");
        itr++; // increment itr should hold the value of "movetime"
        if (itr == args.end())
        {
            LOG_ERROR() << "No movetime specified";
            return std::nullopt;
        }
        searchParams.MoveTime = std::stoi(*itr);
        m_stream << "info movetime " << searchParams.MoveTime << "\n";
        return 1;
    };
    options["infinite"] = []() { LOG_ERROR() << "Not yet implemented"; return std::nullopt; };

    options["depth"] = [&searchParams, this, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "depth");
        itr++; // increment itr should hold the value of "depth"
        if (itr == args.end())
        {
            LOG_ERROR() << "No depth specified";
            return std::nullopt;
        }
        searchParams.SearchDepth = std::stoi(*itr);
        m_stream << "info depth " << searchParams.SearchDepth << "\n";
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

    Move mv = m_context.CalculateBestMove(searchParams);
    m_stream << "bestmove " << mv.toString() << "\n";
    return true;
}