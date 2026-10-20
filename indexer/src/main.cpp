// phant-indexer: reads PGN, from files or standard input, and writes one JSON line per game to standard output,
// see game_index.hpp for what a line holds.

#include "game_index.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <core/build_info.hpp>
#include <system/static_initializer.hpp>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
    constexpr const char* c_usage =
        "usage: phant-indexer [--verbose] [FILE...]\n"
        "       phant-indexer --version\n"
        "\n"
        "Reads the games of each PGN FILE, or of standard input when there's none or FILE is -, and writes one JSON\n"
        "line per game to standard output. --verbose adds each game's moves in UCI notation and its final position.\n";

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
    std::vector<std::string> paths;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--verbose") {
            verbose = true;
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
    const bool fromFiles = !paths.empty();
    if (!fromFiles)
        paths.push_back("-");

    int status = 0;
    std::string input;
    std::string out;
    for (const std::string& path : paths) {
        if (!readAll(path, input)) {
            std::cerr << "phant-indexer: can't read " << path << "\n";
            status = 1;
            continue;
        }

        io::pgn_parser::GameReader reader(input);
        u32 number = 0;
        while (std::optional<io::pgn_parser::Game> game = reader.next()) {
            indexer::GameIndex entry = indexer::index(*game, ++number, verbose);
            if (fromFiles)
                entry.source = path;
            out += indexer::toJson(entry, verbose);
            out += '\n';
            if (out.size() > (1 << 16)) {
                std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
                out.clear();
            }
        }
    }
    std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
    std::cout.flush();
    return status;
}
