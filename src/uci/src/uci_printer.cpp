#include <uci/uci_printer.hpp>

#include <iostream>
#include <memory>

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ostream_sink.h>

namespace io {
namespace printer {

void uciPrinterInit() {
    // Every UCI instance calls this, the logger is global so only the first call creates it.
    if (spdlog::get("uci"))
        return;

    // Synchronous and through std::cout so output is in order, flushed per line, and can be redirected (tests).
    auto cout_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(std::cout, /*force_flush*/ true);
    auto uci_logger = std::make_shared<spdlog::logger>("uci", cout_sink);

    uci_logger->set_level(spdlog::level::info);

    // Only the message, no timestamps or levels, and "\n" rather than the platform default eol.
    uci_logger->set_formatter(std::make_unique<spdlog::pattern_formatter>("%v", spdlog::pattern_time_type::local, "\n"));
    spdlog::register_logger(uci_logger);
}

}  // namespace printer
}  // namespace io
