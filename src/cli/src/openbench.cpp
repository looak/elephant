#include <iostream>

#include <diagnostics/logger.hpp>
#include <system/static_initializer.hpp>
#include <commands/logic/command_processor.hpp>
#include <core/uci_search_listener.hpp>
#include <io/printer.hpp>
#include <search/bench.hpp>

bool g_initialized = static_initializer::initialize();

void runBench() {
    io::printer::uciPrinterInit();

    UciSearchListener listener;
    const bench::Result result = bench::runSignature(listener);

    std::cout << "info string " << result.elapsedMs << " ms\n";
    std::cout << result.nodes << " nodes " << result.nps() << " nps\n";
}

int main(int argc, char* argv[]) {
    assert(g_initialized);

    if (argc > 1) {
        if (std::string(argv[1]) == "bench") {
            runBench();
            return 0;
        }
    }    

    spdlog::info("Starting independent UCI mode...");
    spdlog::set_level(spdlog::level::err);

    UciModeProcessor processor;
    processor.processInput(nullptr, "");

    return 0;
}
