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

std::shared_ptr<spdlog::logger> debug_search_logger() {
    // 1. Thread-Safe Singleton Pattern (ensures logger is only created once)
    static std::once_flag flag;
    static std::shared_ptr<spdlog::logger> logger;

    std::call_once(flag, [] {
        try {
            // --- A. Initialize Asynchronous Queue/Pool (The Performance Key) ---
            // Create an internal thread pool for all async loggers: 8192 queue size, 1 dedicated thread.
            spdlog::init_thread_pool(8192, 1); 

            // --- B. Create the Sink ---
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "logs/search_trace.log", 
                1048576 * 50, // 50MB max file size
                5            // 3 files (trace.log, trace.1.log, trace.2.log)
            );

            // --- C. Create the Asynchronous Logger ---
            logger = std::make_shared<spdlog::async_logger>(
                "search_logger", 
                file_sink, 
                spdlog::thread_pool(), // Use the global thread pool
                spdlog::async_overflow_policy::block // Safer: blocks if queue is full, preventing data loss
            );

            // Configure logging level and flushing
            logger->set_level(spdlog::level::debug);
            logger->flush_on(spdlog::level::info); 

            logger->set_pattern("%v"); // Only the message, no timestamps or levels.
            
            // Register it so it can be retrieved by name, although the function is safer
            spdlog::register_logger(logger);

        } catch (const spdlog::spdlog_ex& ex) {
            // Fail gracefully if file creation fails
            std::cerr << "Search Logger creation failed: " << ex.what() << std::endl;
            // You may want to create a null logger here as a fall back to prevent crashes
        }
    });

    return logger;
}
}

std::string logging::internals::s_logFileName = logging::internals::generateUniqueFilename("log_");
std::string logging::internals::s_coutFileName = logging::internals::generateUniqueFilename("cout_");