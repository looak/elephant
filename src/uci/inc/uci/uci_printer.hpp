#pragma once
#include <utility>

#include <spdlog/spdlog.h>

namespace io {
namespace printer {

void uciPrinterInit();

template<typename... Args>
void uci(fmt::format_string<Args...> fmt, Args&&... args) {
    auto logger = spdlog::get("uci");
    if (logger)
        logger->info(fmt, std::forward<Args>(args)...);
}

// for critical UCI outputs, flush immediately
inline void uci_flush() {
    auto logger = spdlog::get("uci");
    if (logger) {
        logger->flush();
    }
}

}  // namespace printer
}  // namespace io
