#include <cassert>
#include <iostream>
#include <string>

#include <diagnostics/logger.hpp>
#include <system/static_initializer.hpp>
#include <search/bench.hpp>
#include <uci/uci_loop.hpp>
#include <uci/uci_printer.hpp>
#include <uci/uci_search_listener.hpp>

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

    runUciLoop();

    return 0;
}
