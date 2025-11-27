#include <core/uci.hpp>

#include <elephant_gambit_config.h>
#include <io/fen_parser.hpp>
#include <io/san_parser.hpp>
#include <core/game_context.hpp>
#include <move/move.hpp>
#include <search/search.hpp>
#include <util/bench_positions.hpp>

#include <functional>
#include <map>
#include <optional>
#include <string>

UCI::UCI() 
    : m_enabled(true)
    , m_timeManager(SearchParameters{}, Set::WHITE)
    , m_stream(std::cout)
{    
    std::osyncstream output(m_stream);
    output << "id name Elephant Gambit " << ELEPHANT_GAMBIT_VERSION_STR << "\n";
    output << "id author Alexander Loodin Ek\n";    
    InitializeOptions();
}

UCI::~UCI() {
    std::osyncstream output(m_stream);
    output << "quit\n";
}

void UCI::InitializeOptions() 
{
    SetOption({"name", "Threads", "value", "1"});
    SetOption({ "name", "Hash", "value", "8" });
}

void
UCI::Enable()
{
    m_enabled = true;
    std::osyncstream output(m_stream);
    output << "uciok\n";
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
    std::osyncstream output(m_stream);
    output << "readyok\n";
    return true;
}

bool
UCI::SetOption(const std::list<std::string> args)
{
    if (args.size() < 4) {
        throw new ephant::uci_command_exception("option", "Not enough arguments");        
        return false;
    }
    
    auto&& option = args.begin();
    auto&& name = std::next(option);
    auto&& valuetype = std::next(name);
    auto&& value = std::next(valuetype);

    
    if (name->compare("Threads") == 0) {        
        m_options["Threads"] = *value;
        m_threadCount = static_cast<u16>(std::stoi(*value));
    }
    else if (name->compare("Hash") == 0) {
        m_options["Hash"] = *value;
        size_t newSize = static_cast<size_t>(std::stoi(*value));
        m_context.editTranspositionTable().resize(newSize);
    }
    else {
        throw new ephant::uci_command_exception("option", "Unknown option: " + *name);
        return false;
    }

    return true;
}

bool
UCI::Position(std::list<std::string> args)
{
    if (args.size() == 0) {
        throw new ephant::uci_command_exception("position", "No arguments");
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

        if (!io::fen_parser::deserialize(fen.c_str(), m_context.editChessboard())) {            
            return false;
        }
    }

    if (args.size() == 0)
        return true;

    if (args.front() == "moves") {
        PositionReader position = m_context.readChessboard().readPosition();

        args.pop_front();
        while (args.size() > 0) {
            std::string moveStr = args.front();
            args.pop_front();

            PackedMove move = io::san_parser::deserialize(moveStr);
            auto cp = position.pieceAt(move.sourceSqr());

            if (cp == ChessPiece::None()) {
                throw new ephant::uci_command_exception("moves", "Invalid move: " + moveStr);
                return false;
            }

            auto capture = position.pieceAt(move.targetSqr());
            if (capture != ChessPiece::None() 
            || position.enPassant().readSquare() == move.targetSqr()) {
                move.setCapture(true);
            }

            m_context.MakeMove(move);
        }
    }

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
    m_timeManager.cancel();
    return true;
}


bool
UCI::Go(std::list<std::string> args)
{
    SearchParameters searchParams;
    m_timeManager.reset();

    // Typedef for sanity
    using ArgIterator = std::list<std::string>::const_iterator;
    using OptionHandler = std::function<bool(ArgIterator current, ArgIterator end)>;

    // some of these args have values associated with them, so when we iterate
    // over the options, some times we need to jump twice. Hence the lambda
    // returns a optional, since the lambda can also fail.
    std::map<std::string, OptionHandler> options;

    auto parse_int = [&]<typename T>(T& target, std::function<bool()> custom_options = nullptr) {
        return [&](ArgIterator it, ArgIterator end) -> int {
            auto valueIt = std::next(it);
            if (valueIt == end) {
                throw ephant::uci_command_exception(*it, "Expected integer value but none found");
            }
            try {
                i32 value = static_cast<u32>(std::stoi(*valueIt));
                if (value < 0) {
                    throw ephant::uci_command_exception(*it, "Negative integer value not allowed");
                }
                if (custom_options && !custom_options()) {
                    throw ephant::uci_command_exception(*it, "Custom option validation failed");
                }
                return 1; // we consumed one additional argument
            }
            catch (const std::exception& e) {
                throw ephant::uci_command_exception(*it, "Invalid integer value: " + *valueIt);                
            }
        };
    };

    auto parse_bool = [&](bool& target, std::function<bool()> custom_options = nullptr) {
        return [&](ArgIterator it, ArgIterator) -> int {
            target = true;
            if (custom_options && !custom_options()) {
                throw ephant::uci_command_exception(*it, "Custom option validation failed");
            }
            return 0; // boolean flag requires no additional arguments
        };
    };

    auto not_yet_implemented = [](ArgIterator it, ArgIterator) -> int {
        throw ephant::uci_command_exception(*it, "Option not yet implemented");
        return 0;
    };


    options["searchmoves"] = not_yet_implemented;
    options["ponder"] = not_yet_implemented;
    options["nodes"] = not_yet_implemented;
    options["mate"] = not_yet_implemented;
                
    options["depth"] = parse_int(searchParams.SearchDepth);    
    options["wtime"] = parse_int(searchParams.WhiteTimelimit);
    options["btime"] = parse_int(searchParams.BlackTimelimit);
    options["winc"] = parse_int(searchParams.WhiteTimeIncrement);
    options["binc"] = parse_int(searchParams.BlackTimeIncrement);
    options["movestogo"] = parse_int(searchParams.MovesToGo);
    options["movetime"] = parse_int(searchParams.MoveTime);
    options["infinite"] = parse_bool(searchParams.Infinite, [&searchParams]() {
        searchParams.MoveTime = 0;
        searchParams.SearchDepth = 0;
        return true;
    });

    for (auto it = args.begin(); it != args.end(); ++it) {
        auto handler = options.find(*it);
        
        if (handler == options.end()) {
            // Standard practice: Log unknown options but don't crash/throw.
            // GUIs often send extra junk. Be robust.
            LOG_WARN("Unknown option ignored: {}", *it);
            continue; 
        }

        // Pass the context (current position and end) to the handler
        int consumed = handler->second(it, args.end());
        
        // Safety check before advancing
        if (std::distance(it, args.end()) <= consumed) {
            // This theoretically shouldn't happen if the lambda checks bounds, 
            // but trust no one.
            break; 
        }
        
        std::advance(it, consumed);
    }

    Search searcher(m_context);
    searchParams.ThreadCount = m_threadCount;

    std::osyncstream output(m_stream);
    if (m_context.readToPlay() == Set::WHITE) {
        m_timeManager.applyTimeSettings(searchParams, Set::WHITE);
        SearchResult result = searcher.go<Set::WHITE>(searchParams, m_timeManager);
        output << "bestmove " << result.move().toString() << "\n";
    }
    else {
        m_timeManager.applyTimeSettings(searchParams, Set::BLACK);
        SearchResult result = searcher.go<Set::BLACK>(searchParams, m_timeManager);
        output << "bestmove " << result.move().toString() << "\n";
    }
    return true;
}

bool UCI::Bench(std::list<std::string>)
{
    spdlog::info("Starting benchmark...");

    Clock timer;
    timer.Start();
    u64 nodes = 0;

    for (const auto& fen : bench::fens) {
        GameContext context;
        io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

        Search search(context);
        SearchParameters params;
        TimeManager tm(params, context.readToPlay());
        
        params.SearchDepth = bench::depth;
        if (context.readToPlay() == Set::WHITE) {
            auto result = search.go<Set::WHITE>(params, tm);
            nodes += result.count;
        }
        else {
            auto result = search.go<Set::BLACK>(params, tm);
            nodes += result.count;
        }
    }

    timer.Stop();
    float elapsedSeconds = timer.getElapsedSeconds();
    i64 elapsedMilliseconds = timer.getElapsedTime();
    std::cout << "info string " << elapsedSeconds << " seconds\n";
    std::cout << nodes << " nodes " << nodes*1000 / (u64)elapsedMilliseconds << " nps\n";
    return true;
}

bool
UCI::Perft(std::list<std::string>)
{
    return false;
}