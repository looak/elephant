#include <diagnostics/logger.hpp>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#ifdef DEBUG_SEARCH_TREE
#include <spdlog/async.h>
#include <spdlog/details/thread_pool.h>
#endif

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

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

    std::string s_logFileName = generateUniqueFilename("logs/log_");
    std::string s_coutFileName = generateUniqueFilename("logs/cout_");
}

std::string readLogFilename() { return internals::s_logFileName; }
std::string readCoutFilename() { return internals::s_coutFileName; }

void init() {
    auto log_level = static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);

    // Create a multi-threaded console sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(log_level);

    // Create a multi-threaded logger with multiple sinks
    auto mtLogger = std::make_shared<spdlog::logger>(
        "mtLogger", spdlog::sinks_init_list{console_sink});

#ifdef OUTPUT_LOG_TO_FILE
    // Create a multi-threaded rotating file sink

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        readLogFilename(), 10 * 1024 * 1024, 5); // 10MB, 5 backup files
    file_sink->set_level(log_level);

    mtLogger->sinks().push_back(file_sink);
    #endif

    spdlog::set_default_logger(mtLogger);
    spdlog::set_level(log_level);
    spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");

    spdlog::trace("Initialized with log level: {}\n",
            spdlog::level::to_string_view(log_level));
}

void deinit(){
    spdlog::shutdown();
}

#ifdef DEBUG_SEARCH_TREE
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
                "logs/trace/search.log",
                1048576 * 50, // 50MB max file size
                3            // 3 files (trace.log, trace.1.log, trace.2.log)
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
#endif
} // namespace logging
