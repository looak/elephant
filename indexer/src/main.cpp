// phant-indexer: reads PGN, from files or standard input, plain, bz2, gzip or zstd, and writes one JSON line per game
// to standard output, see game_index.hpp for what a line holds; with --positions every position's hash too, and with
// --position the games that reach a position instead, and with --draw the training positions drawn from the games.
// indexer/README.md has the formats.

#include "draw.hpp"
#include "game_index.hpp"
#include "input.hpp"
#include "polyglot.hpp"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <core/build_info.hpp>
#include <core/chessboard.hpp>
#include <io/fen_parser.hpp>
#include <system/static_initializer.hpp>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
    constexpr const char* c_usage =
        "usage: phant-indexer [--verbose] [FILE...]\n"
        "       phant-indexer --positions OUT [--verbose] [FILE]\n"
        "       phant-indexer --position FEN [FILE...]\n"
        "       phant-indexer --draw N [--quiet] [--min-ply P] [--seed S] [FILE...]\n"
        "       phant-indexer --version\n"
        "\n"
        "Reads the games of each PGN FILE, or of standard input when there's none or FILE is -, and writes one JSON\n"
        "line per game to standard output. An input may be bz2, gzip or zstd, told by its first bytes. --verbose\n"
        "adds each game's moves in UCI notation and its final position.\n"
        "\n"
        "--positions OUT also writes every position of every game that read to OUT, 14 bytes each: its Polyglot hash,\n"
        "the game's place in the input, the ply and the move played from it, little-endian; one input, of at most 65535\n"
        "games.\n"
        "--position FEN writes, instead of the entries, a JSON line for each game & ply that reach FEN's position.\n"
        "\n"
        "--draw N writes, instead of the entries, training positions as '<fen> [result]' lines, the result white's: up to\n"
        "N positions of each game that read and has a result, none twice in the run. --quiet keeps the positions that\n"
        "are naturally quiet, not in check and their own quiescence search result; --min-ply P skips the positions before\n"
        "ply P of the game from the initial position (the game phase decides when its FEN has no move number);\n"
        "--seed S (default 1) repeats a run. A summary goes to standard error.\n";

    bool toNumber(const char* text, u64& value)
    {
        const std::string_view view(text);
        const auto [end, error] = std::from_chars(view.data(), view.data() + view.size(), value);
        return error == std::errc{} && end == view.data() + view.size();
    }

    // the --draw summary: how many quiet positions the games yield, on average and at the low end.
    void printDrawSummary(const indexer::DrawStats& stats, const indexer::DrawOptions& options)
    {
        std::vector<u32> quiet = stats.quietPerGame;
        std::sort(quiet.begin(), quiet.end());
        auto at = [&](double fraction) {
            return quiet.empty() ? 0u : quiet[static_cast<size_t>(fraction * double(quiet.size() - 1))];
        };
        const double mean = quiet.empty() ? 0.0 : double(stats.quiet) / double(quiet.size());
        const auto few = std::count_if(quiet.begin(), quiet.end(), [&](u32 n) { return n < options.perGame; });
        std::cerr << std::format("phant-indexer: {} games, {} looked at (read, with a result), {} positions from ply {} with a "
                                 "legal move and not in check, {} {}\n",
            stats.games, stats.drawn, stats.candidates, options.minPly, stats.quiet,
            options.quiet ? "naturally quiet" : "kept (no --quiet)");
        std::cerr << std::format("phant-indexer: positions kept a game: mean {:.1f}, minimum {}, 5th percentile {}, median {}; "
                                 "{} games under {}\n",
            mean, at(0.0), at(0.05), at(0.5), few, options.perGame);
        if (options.minPly > 0) {
            std::cerr << std::format("phant-indexer: {} of {} games have no ply for their start position, the game phase "
                                     "(above {} is early) decided for them\n",
                stats.unknownStart, stats.drawn, indexer::c_earlyPhase);
        }
        std::cerr << std::format("phant-indexer: {} written, {} left out as duplicates\n", stats.written, stats.duplicates);
    }

    // the most games --positions numbers in an input, and the most plies in a game: 2 bytes each.
    constexpr size_t c_maxRecordValue = 0xffff;

    std::string version()
    {
        for (const auto& [key, value] : build_info::entries()) {
            if (key == "version")
                return value;
        }
        return "unknown";
    }

    // the whole input, as bytes: offsets are byte offsets, so nothing translates line endings.
    bool readAll(const std::string& path, std::string& out)
    {
        if (path == "-") {
            out.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
            return !std::cin.bad();
        }
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return false;
        std::ostringstream buffer;
        buffer << file.rdbuf();
        out = std::move(buffer).str();
        return true;
    }
} // namespace

// the move generator's attack tables, as the engine's main builds them.
bool g_initialized = static_initializer::initialize();

int main(int argc, char** argv)
{
    if (!g_initialized) {
        std::cerr << "phant-indexer: the engine's tables didn't initialize\n";
        return 2;
    }
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    std::ios::sync_with_stdio(false);

    bool verbose = false;
    std::string positionsPath;
    std::optional<u64> wanted;  // --position's hash
    std::optional<indexer::DrawOptions> draw;  // --draw
    u64 drawCount = 0, drawMinPly = 0, drawSeed = 1;
    bool drawQuiet = false;
    std::vector<std::string> paths;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--verbose") {
            verbose = true;
        }
        else if (arg == "--quiet") {
            drawQuiet = true;
        }
        else if ((arg == "--draw" || arg == "--min-ply" || arg == "--seed") && i + 1 < argc) {
            u64 value = 0;
            if (!toNumber(argv[++i], value) || (arg == "--draw" && value == 0)) {
                std::cerr << "phant-indexer: " << arg << " needs a number" << (arg == "--draw" ? " above 0" : "") << ", got "
                          << argv[i] << "\n";
                return 2;
            }
            (arg == "--draw" ? drawCount : arg == "--min-ply" ? drawMinPly : drawSeed) = value;
        }
        else if ((arg == "--positions" || arg == "--position" || arg == "--draw" || arg == "--min-ply" || arg == "--seed")
            && i + 1 >= argc) {
            std::cerr << "phant-indexer: " << arg << " needs a value\n" << c_usage;
            return 2;
        }
        else if (arg == "--positions") {
            positionsPath = argv[++i];
        }
        else if (arg == "--position") {
            const std::string fen = argv[++i];
            Chessboard board;
            bool read = false;
            try {
                read = io::fen_parser::deserialize(fen, board);
            }
            catch (const std::exception&) {
                read = false;
            }
            if (!read) {
                std::cerr << "phant-indexer: the FEN doesn't read: " << fen << "\n";
                return 2;
            }
            wanted = indexer::polyglotHash(board);
        }
        else if (arg == "--version") {
            std::cout << "phant-indexer " << version() << " format " << indexer::c_formatVersion << " positions "
                      << indexer::c_positionsVersion << "\n";
            return 0;
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout << c_usage;
            return 0;
        }
        else if (arg.size() > 1 && arg.starts_with("-")) {
            std::cerr << "phant-indexer: unknown option " << arg << "\n" << c_usage;
            return 2;
        }
        else {
            paths.push_back(arg);
        }
    }
    if (drawCount != 0) {
        draw = indexer::DrawOptions{ static_cast<u32>(std::min<u64>(drawCount, 0xffffffffu)),
            static_cast<u32>(std::min<u64>(drawMinPly, 0xffffffffu)), drawQuiet, drawSeed };
    }
    else if (drawQuiet || drawMinPly != 0 || drawSeed != 1) {
        std::cerr << "phant-indexer: --quiet, --min-ply and --seed go with --draw\n" << c_usage;
        return 2;
    }
    if (draw && (!positionsPath.empty() || wanted || verbose)) {
        std::cerr << "phant-indexer: --draw doesn't go with --positions, --position or --verbose\n" << c_usage;
        return 2;
    }
    if (!positionsPath.empty() && wanted) {
        std::cerr << "phant-indexer: --positions and --position don't go together\n" << c_usage;
        return 2;
    }
    // the records name a game by its place in its input only, so they come from one input.
    if (!positionsPath.empty() && paths.size() > 1) {
        std::cerr << "phant-indexer: --positions reads one input\n" << c_usage;
        return 2;
    }
    const bool fromFiles = !paths.empty();
    if (!fromFiles)
        paths.push_back("-");

    std::ofstream positionsFile;
    if (!positionsPath.empty()) {
        positionsFile.open(positionsPath, std::ios::binary | std::ios::trunc);
        if (!positionsFile) {
            std::cerr << "phant-indexer: can't write " << positionsPath << "\n";
            return 1;
        }
    }
    const bool positions = positionsFile.is_open() || wanted.has_value() || draw.has_value();
    std::optional<indexer::Drawer> drawer;
    if (draw)
        drawer.emplace(*draw);

    int status = 0;
    std::string raw;
    std::string input;
    std::string error;
    std::string out;
    std::string records;
    auto flush = [&](bool all) {
        if (all || out.size() > (1 << 16)) {
            std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
            out.clear();
        }
        if (positionsFile.is_open() && (all || records.size() > (1 << 20))) {
            positionsFile.write(records.data(), static_cast<std::streamsize>(records.size()));
            records.clear();
        }
    };
    for (const std::string& path : paths) {
        if (!readAll(path, raw)) {
            std::cerr << "phant-indexer: can't read " << path << "\n";
            status = 1;
            continue;
        }
        // offsets & lengths are into the decompressed PGN.
        if (!indexer::decompress(raw, indexer::detect(raw), input, error)) {
            std::cerr << "phant-indexer: can't decompress " << path << ": " << error << "\n";
            status = 1;
            continue;
        }

        io::pgn_parser::GameReader reader(input);
        u32 number = 0;
        while (std::optional<io::pgn_parser::Game> game = reader.next()) {
            indexer::GameIndex entry = indexer::index(*game, ++number, verbose, positions, draw.has_value() || positionsFile.is_open());
            if (fromFiles)
                entry.source = path;

            if (drawer) {
                drawer->draw(entry, out);
                flush(false);
                continue;
            }

            if (wanted) {
                for (size_t ply = 0; entry.ok && ply < entry.positions.size(); ++ply) {
                    if (entry.positions[ply] != *wanted)
                        continue;
                    out += fromFiles ? std::format("{{\"source\":{},", indexer::toJsonString(path)) : std::string("{");
                    out += std::format("\"game\":{},\"ply\":{}}}\n", entry.number, ply);
                }
                flush(false);
                continue;
            }

            if (positionsFile.is_open() && entry.ok) {
                if (number > c_maxRecordValue || entry.positions.size() > c_maxRecordValue + 1) {
                    flush(true);
                    std::cerr << std::format("phant-indexer: --positions stops at game {} of {}: its records hold at "
                                             "most {} games an input and {} plies a game\n",
                        number, path, c_maxRecordValue, c_maxRecordValue);
                    return 1;
                }
                for (size_t ply = 0; ply < entry.positions.size(); ++ply) {
                    const PackedMove move = ply < entry.played.size() ? entry.played[ply] : PackedMove::NullMove();
                    indexer::appendPositionRecord(records, entry.positions[ply], number, static_cast<u32>(ply), move);
                }
            }
            out += indexer::toJson(entry, verbose);
            out += '\n';
            flush(false);
        }
    }
    flush(true);
    std::cout.flush();
    if (drawer)
        printDrawSummary(drawer->stats(), *draw);
    if (positionsFile.is_open()) {
        positionsFile.close();
        if (!positionsFile) {
            std::cerr << "phant-indexer: can't write " << positionsPath << "\n";
            return 1;
        }
    }
    return status;
}
