#include <core/uci.hpp>

#include <elephant_gambit_config.h>
#include <io/fen_parser.hpp>
#include <io/san_parser.hpp>
#include <io/printer.hpp>
#include <core/game_context.hpp>
#include <math/bench_positions.hpp>
#include <move/move.hpp>
#include <search/search.hpp>
#include <system/clock.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <thread>

namespace {
// A bad command never ends the engine, the GUI is told through an info string and the log keeps it.
void reportError(const std::exception& e)
{
    LOG_WARN("{}", e.what());
    io::printer::uci("info string error {}", e.what());
    io::printer::uci_flush();
}

// long algebraic notation as UCI sends it, e2e4 or e7e8q.
bool looksLikeUciMove(const std::string& token)
{
    auto isFile = [](char c) { return c >= 'a' && c <= 'h'; };
    auto isRank = [](char c) { return c >= '1' && c <= '8'; };
    if (token.length() != 4 && token.length() != 5)
        return false;
    if (!isFile(token[0]) || !isRank(token[1]) || !isFile(token[2]) || !isRank(token[3]))
        return false;
    return token.length() == 4 || std::string_view("qrbnQRBN").find(token[4]) != std::string_view::npos;
}
}  // namespace

UCI::UCI()
    : m_enabled(true)
    , m_timeManager(SearchParameters{}, Set::WHITE)
{
    io::printer::uciPrinterInit();
    io::printer::uci("id name Elephant Gambit {}", ELEPHANT_GAMBIT_VERSION_FULL);
    io::printer::uci("id author Alexander Loodin Ek");
    InitializeOptions();
}

UCI::~UCI() {
    io::printer::uci("quit");
    io::printer::uci_flush();
}

void UCI::InitializeOptions() {
    SetOption({"name", "Threads", "value", "1"});
    SetOption({ "name", "Hash", "value", "8" });
    SetOption({ "name", "Move", "Overhead", "value", std::to_string(c_defaultMoveOverhead_ms) });
}

void
UCI::Enable() {
    m_enabled = true;
    io::printer::uci("uciok");
    io::printer::uci_flush();
}

bool
UCI::Enabled() const {
    return m_enabled;
}

void
UCI::Disable() {
    m_enabled = false;
}

bool
UCI::IsReady() const {
    io::printer::uci("readyok");
    io::printer::uci_flush();
    return true;
}

bool
UCI::SetOption(const std::list<std::string> args)
{
    try {
        if (args.size() < 4)
            throw ephant::uci_command_exception("option", "Not enough arguments");

        // option names may contain spaces, e.g. "name Move Overhead value 10"
        auto token = std::next(args.begin());
        std::string name = *token++;
        while (token != args.end() && *token != "value")
            name += " " + *token++;

        if (token == args.end() || std::next(token) == args.end())
            throw ephant::uci_command_exception("option", "No value given for option: " + name);
        const std::string& value = *std::next(token);
        auto toInt = [&name, &value]() {
            try {
                return std::stoi(value);
            }
            catch (const std::exception&) {
                throw ephant::uci_command_exception("option", "Invalid value for option " + name + ": " + value);
            }
        };

        if (name == "Threads") {
            u16 threadCount = static_cast<u16>(std::clamp(toInt(), 1, 64));
            m_options["Threads"] = value;
            m_threadCount = threadCount;
        }
        else if (name == "Hash") {
            size_t newSize = static_cast<size_t>(std::max(1, toInt()));
            m_options["Hash"] = value;
            m_context.editTranspositionTable().resize(newSize);
        }
        else if (name == "Move Overhead") {
            u32 overhead = static_cast<u32>(std::max(0, toInt()));
            m_options["Move Overhead"] = value;
            m_timeManager.setMoveOverhead(overhead);
        }
        else {
            throw ephant::uci_command_exception("option", "Unknown option: " + name);
        }
    }
    catch (const std::exception& e) {
        // also a Hash size that can't be allocated.
        reportError(e);
        return false;
    }

    return true;
}

bool
UCI::Position(std::list<std::string> args)
{
    try {
        return ParsePosition(std::move(args));
    }
    catch (const std::exception& e) {
        reportError(e);
        return false;
    }
}

bool
UCI::ParsePosition(std::list<std::string> args)
{
    if (args.size() == 0)
        throw ephant::uci_command_exception("position", "No arguments");

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

        if (!io::fen_parser::deserialize(fen.c_str(), m_context.editChessboard()))
            throw ephant::uci_command_exception("position", "Invalid fen: " + fen);
    }
    else {
        throw ephant::uci_command_exception("position", "Expected startpos or fen, got: " + arg);
    }

    if (args.size() == 0)
        return true;

    if (args.front() == "moves") {
        PositionReader position = m_context.readChessboard().readPosition();

        args.pop_front();
        while (args.size() > 0) {
            std::string moveStr = args.front();
            args.pop_front();
            if (moveStr.empty())
                continue;  // a trailing space splits into an empty token.
            if (!looksLikeUciMove(moveStr))
                throw ephant::uci_command_exception("moves", "Invalid move: " + moveStr);

            PackedMove move = io::san_parser::deserialize(moveStr);
            auto cp = position.pieceAt(move.sourceSqr());

            if (cp == ChessPiece::None())
                throw ephant::uci_command_exception("moves", "Invalid move: " + moveStr);

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
    // a search still running would race the clear below, GUIs stop it first but make sure.
    SyncGo();
    m_context.NewGame();
    // games are independent, nothing from the last game may carry over. The TT (and its age) is the only search state
    // that outlives a go, the killers & history are built per search in ThreadSearchContext. Not part of
    // GameContext::NewGame, "position startpos" calls that every move.
    m_context.editTranspositionTable().clear();
    return true;
}

bool
UCI::Stop()
{
    m_timeManager.cancel();
    return true;
}

bool
UCI::PonderHit()
{
    m_timeManager.cancel();
    return true;
}

bool
UCI::AsyncGo(std::list<std::string> args)
{
    SyncGo(); // should stop any ongoing search first -- but we still check to be sure.

    if (m_isSearching.load()) {
        reportError(ephant::uci_command_exception("go", "Search already in progress"));
        return false;
    }
    m_isSearching.store(true);

    m_searchFuture = std::async(std::launch::async, [this, args]() {
        try {
            this->Go(args);
        }
        catch (const std::exception& e) {
            // the GUI waits for a bestmove whatever happened.
            reportError(e);
            io::printer::uci("bestmove 0000");
            io::printer::uci_flush();
            m_isSearching.store(false);
        }
    });

    return true;
}

void
UCI::SyncGo()
{
    m_timeManager.cancel(); // Ensure any ongoing search is stopped
    if (m_searchFuture.valid()) {
        try{
            m_searchFuture.get(); // wait
        }
        catch (const std::exception&  e) {
            spdlog::error("Exception in async search: {}", e.what());
        }
    }
}

bool
UCI::Go(std::list<std::string> args)
{
    SearchParameters searchParams;
    m_timeManager.reset();

    // Typedef for sanity
    using ArgIterator = std::list<std::string>::const_iterator;
    // returns how many arguments after the option it consumed.
    using OptionHandler = std::function<int(ArgIterator current, ArgIterator end)>;

    // some of these args have values associated with them, so when we iterate
    // over the options, some times we need to jump twice. Hence the lambda
    // returns a optional, since the lambda can also fail.
    std::map<std::string, OptionHandler> options;

    auto parse_int = [&]<typename T>(T& target, std::optional<std::function<bool()>> custom_options = std::nullopt) {
        return [&target, custom_options](ArgIterator it, ArgIterator end) -> int {
            auto valueIt = std::next(it);
            if (valueIt == end) {
                throw ephant::uci_command_exception(*it, "Expected integer value but none found");
            }
            i64 value = 0;
            try {
                value = std::stoll(*valueIt);
            }
            catch (const std::exception&) {
                throw ephant::uci_command_exception(*it, "Invalid integer value: " + *valueIt);
            }
            // a negative clock happens when a GUI lets us run out of time. 0 means "no limit" for most of these, so a
            // negative value becomes the smallest real limit instead.
            if (value < 0)
                value = 1;
            // saturate rather than wrap, "depth 300" into a u8 would otherwise become 44.
            target = static_cast<u64>(value) > static_cast<u64>(std::numeric_limits<T>::max())
                ? std::numeric_limits<T>::max() : static_cast<T>(value);
            if (custom_options.has_value() && custom_options.value()() == false) {
                throw ephant::uci_command_exception(*it, "Custom option validation failed");
            }
            return 1; // we consumed one additional argument
        };
    };

    auto parse_bool = [&](bool& target, std::optional<std::function<bool()>> custom_options = std::nullopt) {
        return [&target, custom_options](ArgIterator it, ArgIterator) -> int {
            target = true;
            if (custom_options.has_value() && custom_options.value()() == false) {
                throw ephant::uci_command_exception(*it, "Custom option validation failed");
            }
            return 0; // boolean flag requires no additional arguments
        };
    };

    // searchmoves takes every move that follows it, legality is checked against the root position by the search.
    options["searchmoves"] = [&searchParams](ArgIterator it, ArgIterator end) -> int {
        int consumed = 0;
        for (auto moveIt = std::next(it); moveIt != end && looksLikeUciMove(*moveIt); ++moveIt) {
            searchParams.SearchMoves.push_back(io::san_parser::deserialize(*moveIt));
            ++consumed;
        }
        return consumed;
    };

    // mate N, a mate in N moves is at most 2N - 1 plies away. Iterative deepening already ends on any forced mate, a
    // depth limit of 2N plies makes sure the search ends when there is no such mate.
    u32 mateInMoves = 0;
    options["mate"] = parse_int(mateInMoves);
    options["nodes"] = parse_int(searchParams.NodeLimit);
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
    // pondering searches like infinite, on the opponent's time. Both stop and ponderhit end it, ponderhit plays the
    // move found so far at once rather than continuing on our own clock.
    options["ponder"] = parse_bool(searchParams.Infinite, [&searchParams]() {
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

        // Pass the context (current position and end) to the handler. A bad value is reported and skipped, the search
        // still runs with the other limits so the GUI gets its bestmove.
        int consumed = 0;
        try {
            consumed = handler->second(it, args.end());
        }
        catch (const std::exception& e) {
            reportError(e);
            continue;
        }

        // Safety check before advancing
        if (std::distance(it, args.end()) <= consumed) {
            // This theoretically shouldn't happen if the lambda checks bounds, 
            // but trust no one.
            break; 
        }
        
        std::advance(it, consumed);
    }

    if (mateInMoves > 0) {
        const u8 matePlies = static_cast<u8>(2 * std::min<u32>(mateInMoves, 127));
        searchParams.SearchDepth = searchParams.SearchDepth == 0 ? matePlies : std::min(searchParams.SearchDepth, matePlies);
    }

    Search searcher(m_context);
    searchParams.ThreadCount = m_threadCount;

    SearchResult result;
    if (m_context.readToPlay() == Set::WHITE) {
        m_timeManager.applyTimeSettings(searchParams, Set::WHITE);
        result = searcher.go<Set::WHITE>(searchParams, m_timeManager);
    }
    else {
        m_timeManager.applyTimeSettings(searchParams, Set::BLACK);
        result = searcher.go<Set::BLACK>(searchParams, m_timeManager);
    }

    // UCI: an infinite or ponder search may only send bestmove after "stop" or "ponderhit", also when it ended on its
    // own (mate, ply limit).
    if (searchParams.Infinite) {
        std::stop_token stopToken = m_timeManager.cancelToken();
        while (stopToken.stop_requested() == false)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    io::printer::uci("bestmove {}", result.move().toString());
    io::printer::uci_flush();

    m_isSearching.store(false); // mark search as completed
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
    io::printer::uci("info string {} seconds", elapsedSeconds);
    io::printer::uci("info string {} nodes {} nps", nodes, nodes*1000 / (u64)elapsedMilliseconds);
    return true;
}

bool
UCI::Perft(std::list<std::string>)
{
    return false;
}