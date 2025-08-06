#include "uci.hpp"

#include "elephant_gambit_config.h"
#include <serializing/fen_parser.hpp>
#include "game_context.h"
#include <move/move.hpp>
#include "search.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>

UCI::UCI() :
    m_enabled(true),
    m_stream(std::cout)
{
    m_stream << "id name Elephant Gambit " << ELEPHANT_GAMBIT_VERSION_STR << "\n";
    m_stream << "id author Alexander Loodin Ek\n";
    InitializeOptions();
}

UCI::~UCI() { m_stream << "quit\n"; }

void UCI::InitializeOptions() 
{
    SetOption({"name", "Threads", "value", "1"});
    SetOption({ "name", "Hash", "value", "8" });
}

void
UCI::Enable()
{
    m_enabled = true;
    m_stream << "uciok\n";
}

bool
UCI::Enabled()
{
    return m_enabled;
}

void
UCI::Disable()
{
    m_enabled = false;
}

bool
UCI::IsReady()
{
    m_stream << "readyok\n";
    return true;
}

bool
UCI::SetOption(const std::list<std::string>& args)
{
    if (args.size() < 4) {
        LOG_ERROR() << "SetOption: Not enough arguments";
        return false;
    }

    auto&& option = args.begin();
    auto&& name = std::next(option);
    auto&& valuetype = std::next(name);
    auto&& value = std::next(valuetype);

    if (name->compare("Threads") == 0) {
        LOG_DEBUG() << "Threads: " << *value;
    }
    else if (name->compare("Hash") == 0) {
        m_options["Hash"] = *value;
        m_context.editTranspositionTable().resize(std::stoi(*value));
    }
    else {
        LOG_ERROR() << "Unknown option: " << *name;
        return false;
    }

    return true;

}

bool
UCI::Position(std::list<std::string>& args)
{
    if (args.size() == 0) {
        LOG_ERROR() << "PositionCommand: No arguments";
        return false;
    }

    auto&& arg = args.front();

    if (arg == "startpos") {
        args.pop_front();
        m_context.NewGame();
    }
    else if (arg == "fen") {
        args.pop_front();
        std::string fen = "";
        while (args.size() > 0 && args.front() != "moves" && args.front().find(";") == std::string::npos) {
            fen += args.front() + " ";
            args.pop_front();
        }

        if (!FENParser::deserialize(fen.c_str(), m_context)) {
            LOG_ERROR() << "Failed to parse fen: " << fen;
            return false;
        }
    }

    if (args.size() == 0)
        return true;

    if (args.front() == "moves") {
        Chessboard& board = m_context.editChessboard();

        args.pop_front();
        while (args.size() > 0) {
            std::string moveStr = args.front();
            args.pop_front();

            Move move = Move::fromString(moveStr);
            move.Piece = board.readPieceAt(move.SourceSquare.toSquare());

            if (move.Piece == ChessPiece::None())
            {
                LOG_ERROR() << "Invalid move: " << moveStr;
                return false;
            }

            move.setCapture(m_context.readChessboard().readPieceAt(move.TargetSquare.toSquare()).isValid());

            if (move.Piece.getType() == PieceType::PAWN && board.readPosition().readEnPassant())
            {
                bool enPassant = board.readPosition().readEnPassant().readSquare() == move.TargetSquare.toSquare();
                if (enPassant) {
                    move.setEnPassant(enPassant);
                    move.setCapture(enPassant);
                }
            }

            if (!m_context.MakeMove(move.readPackedMove())) {
                LOG_ERROR() << "Failed to make move: " << move.toString();
                return false;
            }
        }
    }

    // if (args.front().find(";") != std::string::npos)
    // {
    //     std::vector<std::pair<int, int>> depthNodeCount;
    //     do {
    //         std::string depthValue = args.front();
    //         args.pop_front();
    //         std::string nodeCountStr = args.front();
    //         args.pop_front();

    //         depthValue.erase(0, 2);
    //         int depth = std::stoi(depthValue);
    //         int nodeCount = std::stoi(nodeCountStr);

    //         depthNodeCount.push_back(std::make_pair(depth, nodeCount));

    //     } while (args.size() > 0 && args.front().find(";") != std::string::npos);

    //     for (auto&& [depth, nodeCount] : depthNodeCount)
    //     {
    //         Search search;
    //         auto result = search.PerftDivide(m_context, depth);
    //         //LOG_DEBUG() << "Depth: " << depth << " NodeCount: " << nodeCount << "\n";
    //         std::cout << "nodes " << result.Nodes << "\n";
    //     }
    // }

    return true;
}

bool
UCI::NewGame()
{
    m_context.NewGame();
    return true;
}

bool
UCI::Stop()
{
    return true;
}

bool
UCI::Go(std::list<std::string>& args)
{
    SearchParameters searchParams;

    // some of these args have values associated with them, so when we iterate
    // over the options, some times we need to jump twice. Hence the lambda
    // returns a optional, since the lambda can also fail.
    std::map<std::string, std::function<std::optional<int>(void)>> options;
    options["searchmoves"] = []() {
        LOG_ERROR() << "Not yet implemented";
        return std::nullopt;
        };
    options["ponder"] = []() {
        LOG_ERROR() << "Not yet implemented";
        return std::nullopt;
        };
    options["wtime"] = [&searchParams, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "wtime");
        itr++;  // increment itr should hold the value of "movetime"
        if (itr == args.end()) {
            LOG_ERROR() << "No time specified";
            return std::nullopt;
        }
        searchParams.WhiteTimelimit = std::stoi(*itr);
        LOG_DEBUG() << "wtime " << searchParams.WhiteTimelimit << "\n";
        return 1;
        };
    options["btime"] = [&searchParams, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "btime");
        itr++;  // increment itr should hold the value of "movetime"
        if (itr == args.end()) {
            LOG_ERROR() << "No time specified";
            return std::nullopt;
        }
        searchParams.BlackTimelimit = std::stoi(*itr);
        LOG_DEBUG() << "btime " << searchParams.BlackTimelimit << "\n";
        return 1;
        };
    options["winc"] = [&searchParams, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "winc");
        itr++;  // increment itr should hold the value of "movetime"
        if (itr == args.end()) {
            LOG_ERROR() << "No time specified";
            return std::nullopt;
        }
        searchParams.WhiteTimeIncrement = std::stoi(*itr);
        LOG_DEBUG() << "winc " << searchParams.WhiteTimeIncrement << "\n";
        return 1;
        };
    options["binc"] = [&searchParams, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "binc");
        itr++;  // increment itr should hold the value of "movetime"
        if (itr == args.end()) {
            LOG_ERROR() << "No time specified";
            return std::nullopt;
        }
        searchParams.BlackTimeIncrement = std::stoi(*itr);
        LOG_DEBUG() << "binc " << searchParams.BlackTimeIncrement << "\n";
        return 1;
        };
    options["movestogo"] = [&searchParams, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "movestogo");
        itr++;  // increment itr should hold the value of "movestogo"
        if (itr == args.end()) {
            LOG_ERROR() << "No movestogo specified";
            return std::nullopt;
        }
        searchParams.MovesToGo = std::stoi(*itr);
        LOG_DEBUG() << "movestogo " << searchParams.MovesToGo << "\n";
        return 1;
        };
    options["nodes"] = []() {
        LOG_ERROR() << "Not yet implemented";
        return std::nullopt;
        };
    options["mate"] = []() {
        LOG_ERROR() << "Not yet implemented";
        return std::nullopt;
        };
    options["movetime"] = [&searchParams, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "movetime");
        itr++;  // increment itr should hold the value of "movetime"
        if (itr == args.end()) {
            LOG_ERROR() << "No movetime specified";
            return std::nullopt;
        }
        searchParams.MoveTime = std::stoi(*itr);
        LOG_DEBUG() << "movetime " << searchParams.MoveTime << "\n";
        return 1;
        };
    options["infinite"] = [&searchParams]() -> std::optional<int> {
        searchParams.MoveTime = 0;
        searchParams.SearchDepth = 0;
        searchParams.Infinite = true;
        return 0;
        };

    options["depth"] = [&searchParams, args]() -> std::optional<int> {
        auto itr = std::find(args.begin(), args.end(), "depth");
        itr++;  // increment itr should hold the value of "depth"
        if (itr == args.end()) {
            LOG_ERROR() << "No depth specified";
            return std::nullopt;
        }
        searchParams.SearchDepth = std::stoi(*itr);
        LOG_DEBUG() << "depth " << searchParams.SearchDepth << "\n";
        return 1;
        };

    for (auto argItr = args.begin(); argItr != args.end(); ++argItr) {
        auto itr = options.find(*argItr);
        if (itr == options.end()) {
            LOG_ERROR() << "Unknown option: " << *argItr;
            return false;
        }
        std::optional<int> optinalResult = itr->second();
        if (optinalResult) {
            int increments = *optinalResult;
            argItr = std::next(argItr, increments);
            if (argItr == args.end())
                break;
        }
        else {
            LOG_ERROR() << "Invalid option: " << *argItr;
            return false;
        }
    }

    SearchResult result = m_context.CalculateBestMove(searchParams);
    m_stream << "bestmove " << result.move.toString();
    m_stream << "\n";
    return true;
}

bool
UCI::Perft(std::list<std::string>&)
{
    return false;
}