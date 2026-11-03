#include <uci/uci.hpp>
#include <uci/uci_printer.hpp>

#include <elephant_gambit_config.h>
#include <core/build_info.hpp>
#include <diagnostics/exceptions.hpp>
#include <diagnostics/logger.hpp>
#include <io/fen_parser.hpp>
#include <io/san_parser.hpp>
#include <io/printer.hpp>
#include <core/game.hpp>
#include <move/move.hpp>
#include <search/bench.hpp>

#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <string>

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
{
    io::printer::uciPrinterInit();
    io::printer::uci("id name Elephant Gambit {}", ELEPHANT_GAMBIT_VERSION_FULL);
    io::printer::uci("id author Alexander Loodin Ek");
}

UCI::~UCI() {
    SyncGo();
    io::printer::uci("quit");
    io::printer::uci_flush();
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

        // A valid option ends a running search first, see Monad. GUIs only set options while idle, a bad one is
        // reported here without touching the search.
        if (name == "Threads") {
            m_monad.setThreads(static_cast<u16>(std::clamp(toInt(), 1, 64)));
        }
        else if (name == "Hash") {
            m_monad.setHash(static_cast<size_t>(std::max(1, toInt())));
        }
        else if (name == "Move Overhead") {
            m_monad.setMoveOverhead(static_cast<u32>(std::max(0, toInt())));
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
        m_game = Game{};
    }
    else if (arg == "fen") {
        args.pop_front();
        std::string fen = "";
        while (args.size() > 0 && args.front() != "moves" && args.front().find(";") == std::string::npos) {
            fen += args.front() + " ";
            args.pop_front();
        }

        // a new game like startpos's above, GUIs send the whole game again every turn.
        if (!m_game.loadFen(fen))
            throw ephant::uci_command_exception("position", "Invalid fen: " + fen);
    }
    else {
        throw ephant::uci_command_exception("position", "Expected startpos or fen, got: " + arg);
    }

    if (args.size() == 0)
        return true;

    if (args.front() == "moves") {
        PositionReader position = m_game.board().readPosition();

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
            if (capture != ChessPiece::None()) {
                move.setCapture(true);
            }
            // only a pawn captures en passant, any other piece stepping onto the en passant square is a quiet move.
            else if (cp.getType() == PieceType::PAWN && position.enPassant()
                     && position.enPassant().readSquare() == move.targetSqr()) {
                move.setEnPassant(true);  // sets both capture & en passant
            }

            m_game.makeMove(move);
        }
    }

    return true;
}

bool
UCI::NewGame()
{
    // Not part of Game::NewGame, "position startpos" calls that every move and must keep the tables.
    m_monad.newGame();
    m_game = Game{};
    return true;
}

bool
UCI::Stop()
{
    m_monad.stop();
    return true;
}

bool
UCI::PonderHit()
{
    m_monad.stop();
    return true;
}

bool
UCI::AsyncGo(std::list<std::string> args)
{
    // before reading the arguments, a bad go answers after the bestmove of the search it replaces.
    SyncGo();

    std::optional<SearchRequest> request;
    try {
        request = prepareSearch(args);
    }
    catch (const std::exception& e) {
        m_listener.onSearchFailed(e);
        return false;
    }

    m_monad.go(std::move(*request), m_listener);
    return true;
}

void
UCI::SyncGo()
{
    m_monad.stop();
    (void)m_monad.wait();
}

bool
UCI::Go(std::list<std::string> args)
{
    m_monad.go(prepareSearch(args), m_listener);
    (void)m_monad.wait();
    return true;
}

SearchRequest
UCI::prepareSearch(const std::list<std::string>& args)
{
    SearchLimits searchParams;

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

    return makeSearchRequest(m_game, std::move(searchParams));
}

bool UCI::Bench(std::list<std::string>)
{
    spdlog::info("Starting benchmark...");

    UciSearchListener listener;
    const bench::Result result = bench::runSignature(listener);

    io::printer::uci("info string {} seconds", static_cast<float>(result.elapsedMs) / 1000.0f);
    io::printer::uci("info string {} nodes {} nps", result.nodes, result.nps());
    io::printer::uci_flush();
    return true;
}

bool
UCI::BuildInfo() const
{
    for (const auto& [key, value] : build_info::entries()) {
        if (!value.empty())
            io::printer::uci("buildinfo {} {}", key, value);
    }
    io::printer::uci_flush();
    return true;
}

bool
UCI::Perft(std::list<std::string>)
{
    return false;
}