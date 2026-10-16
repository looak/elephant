// ElephantTuner, the offline evaluation tuner, see tuner/CMakeLists.txt.
//
//   ElephantTuner <dataset>      a text file, one position per line: a FEN and the game's result, see dataset.hpp
//
// Loads & checks the dataset and reports what it holds. Tracing the evaluation and fitting its values come next.

#include <exception>
#include <format>
#include <iostream>
#include <string>

#include <dataset.hpp>
#include <system/static_initializer.hpp>

bool g_initialized = static_initializer::initialize();

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: ElephantTuner <dataset>\n"
                     "  dataset: one position per line, a FEN and the game's result from white's side,\n"
                     "           e.g. [1.0], c9 \"1/2-1/2\"; or a bare 0-1\n";
        return 2;
    }

    const std::string path = argv[1];
    tuner::LoadReport report;
    std::vector<tuner::TuningPosition> positions;
    try {
        positions = tuner::loadDataset(path, report);
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    std::size_t whiteWins = 0, draws = 0, blackWins = 0;
    for (const tuner::TuningPosition& position : positions) {
        if (position.result == 1.0)
            ++whiteWins;
        else if (position.result == 0.5)
            ++draws;
        else
            ++blackWins;
    }

    std::cout << std::format("{}: {} positions from {} lines\n", path, positions.size(), report.lines);
    std::cout << std::format("  results    white {}  draw {}  black {}\n", whiteWins, draws, blackWins);
    std::cout << std::format("  not used   {} empty or comment, {} without a result, {} invalid FEN\n", report.skipped,
                             report.noResult, report.badFen);
    for (const std::string& error : report.firstErrors)
        std::cout << "    line " << error << '\n';

    return positions.empty() ? 1 : 0;
}
