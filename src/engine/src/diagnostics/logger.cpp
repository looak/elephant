#include <diagnostics/logger.hpp>

namespace logging {
namespace internals {

    std::string generateUniqueFilename(std::string prefix = "output_")
    {
        auto now = std::chrono::system_clock::now();
        std::time_t timetNow = std::chrono::system_clock::to_time_t(now);
        std::tm* localtime = std::localtime(&timetNow);
        
        std::ostringstream filename;
        filename << prefix << std::put_time(localtime, "%Y%m%d_%H%M%S") << ".log";
        
        return filename.str();
    }

}
}

std::string logging::internals::s_logFileName = logging::internals::generateUniqueFilename("log_");
std::string logging::internals::s_coutFileName = logging::internals::generateUniqueFilename("cout_");