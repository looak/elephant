// phant-indexer: reads PGN, from files or standard input, plain, bz2, gzip or zstd, and writes one JSON line per game
// to standard output, see game_index.hpp for what a line holds; with --positions every position's hash too, and with
// --position the games that reach a position instead. indexer/README.md has the formats.

#include "game_index.hpp"
#include "input.hpp"
#include "polyglot.hpp"

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
        "       phant-indexer --version\n"
        "\n"
        "Reads the games of each PGN FILE, or of standard input when there's none or FILE is -, and writes one JSON\n"
        "line per game to standard output. An input may be bz2, gzip or zstd, told by its first bytes. --verbose\n"
        "adds each game's moves in UCI notation and its final position.\n"
        "\n"
        "--positions OUT also writes every position of every game that read to OUT, 12 bytes each: its Polyglot hash,\n"
        "the game's place in the input and the ply, little-endian; one input, of at most 65535 games.\n"
        "--position FEN writes, instead of the entries, a JSON line for each game & ply that reach FEN's position.\n";

    // the most games --positions numbers in an input, and the most plies in a game: 2 bytes each.
    constexpr size_t c_maxRecordValue = 0xffff;

    // a position record: the hash, the game's place in its input, the ply, little-endian.
    void appendRecord(std::string& out, u64 hash, u32 game, size_t ply)
    {
        for (int i = 0; i < 8; ++i)
            out += static_cast<char>((hash >> (8 * i)) & 0xff);
        for (u64 value : { u64(game), u64(ply) }) {
            out += static_cast<char>(value & 0xff);
            out += static_cast<char>((value >> 8) & 0xff);
        }
    }

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
    std::vector<std::string> paths;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--verbose") {
            verbose = true;
        }
        else if ((arg == "--positions" || arg == "--position") && i + 1 >= argc) {
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
            std::cout << "phant-indexer " << version() << " format " << indexer::c_formatVersion << "\n";
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
    const bool positions = positionsFile.is_open() || wanted.has_value();

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
            indexer::GameIndex entry = indexer::index(*game, ++number, verbose, positions);
            if (fromFiles)
                entry.source = path;

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
                for (size_t ply = 0; ply < entry.positions.size(); ++ply)
                    appendRecord(records, entry.positions[ply], number, ply);
            }
            out += indexer::toJson(entry, verbose);
            out += '\n';
            flush(false);
        }
    }
    flush(true);
    std::cout.flush();
    if (positionsFile.is_open()) {
        positionsFile.close();
        if (!positionsFile) {
            std::cerr << "phant-indexer: can't write " << positionsPath << "\n";
            return 1;
        }
    }
    return status;
}
