#include <diagnostics/assert.hpp>
#include <diagnostics/logger.hpp>

namespace ephant {

ephsert::ephsert(char const* expression, const std::string& prefix, const std::string& file, int line,
                 const std::string& message)
{
    spdlog::critical("{}{}:{} Assertion failed: {} > {}", prefix, file, line, expression, message);
    // abort doesn't flush.
    spdlog::default_logger_raw()->flush();
    std::abort();
}

}  // namespace ephant
