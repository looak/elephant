// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021  Alexander Loodin Ek

// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.If not, see < http://www.gnu.org/licenses/>.
#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <time.h>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <syncstream>
#include <sstream>
#include <string>

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define LOG_TRACE(...)    SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

namespace logging {
namespace internals {

extern std::string s_logFileName;
extern std::string s_coutFileName;
class DualStreamBuffer : public std::streambuf {
public:
    DualStreamBuffer(std::streambuf* one, std::streambuf* two) :
        m_bufferOne(one),
        m_bufferTwo(two)
    {
    }

protected:
    int sync() override
    {
        // int result1 = m_bufferOne->pubsync();
        // int result2 = m_bufferTwo->pubsync();
        // return (result1 == 0 && result2 == 0) ? 0 : -1;
        return 0;
    }

    std::streamsize xsputn(const char* s, std::streamsize n) override
    {
        m_bufferOne->sputn(s, n);
        m_bufferTwo->sputn(s, n);
        return n;
    }

    int overflow(int c) override
    {
        if (c != EOF) {
            m_bufferOne->sputc(static_cast<char>(c));
            m_bufferTwo->sputc(static_cast<char>(c));
        }
        return c;
    }

private:
    std::streambuf* m_bufferOne;
    std::streambuf* m_bufferTwo;
};
} // namespace internals

inline std::string readLogFilename() { return internals::s_logFileName; };
inline std::string readCoutFilename() { return internals::s_coutFileName; };

inline void init() {    
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

inline void deinit(){
    spdlog::shutdown();
}

/**
 * @brief Scoped Redirect, will redirect given outstream to file & it's original target.  */
class ScopedDualRedirect {
public:
    ScopedDualRedirect(std::ostream& stream, const std::string& filename) :
        m_originalStream(stream),
        m_originalBuffer(stream.rdbuf()),
        m_fileStream(filename, std::ios::app)
    {
        m_dualBuffer = new internals::DualStreamBuffer(m_originalStream.rdbuf(), m_fileStream.rdbuf());
        m_originalStream.rdbuf(m_dualBuffer);
    }
    ~ScopedDualRedirect()
    {
        m_originalStream.rdbuf(m_originalBuffer);
        delete m_dualBuffer;
    }

private:
    std::ostream& m_originalStream;
    std::streambuf* m_originalBuffer;
    std::ofstream m_fileStream;
    internals::DualStreamBuffer* m_dualBuffer;
};

} // namespace logging

namespace ephant {
class ephsert final {
public:
    ephsert(char const* expression, const std::string& prefix, const std::string& file, int line, const std::string& message = "")
    {
        spdlog::critical("{}{}:{} Assertion failed: {} > {}", prefix, file, line, expression, message);
    }

    ~ephsert() {
        abort();
    }
};

}  // namespace ephant


// @brief Asserts that the expression evaluates to true and logs a fatal assert message with the expression, file name and line
// number if it fails.
#if defined(ASSERTIONS_ENABLED)
#define ASSERT_MSG(expr, message) \
    if ((expr) != 0) {     \
        int ___noop = 5;   \
        (void)___noop;     \
    }                      \
    else                   \
        ephant::ephsert(#expr, "[ASSERT] ", __FILENAME__, __LINE__, message);
#else
#define ASSERT_MSG(expr, message) \
    switch (0)    \
    case 0:       \
    default:      \
        ;
#endif

#if defined(ASSERTIONS_ENABLED)
#define ASSERT(expr) \
    if ((expr) != 0) {     \
        int ___noop = 5;   \
        (void)___noop;     \
    }                      \
    else                   \
        ephant::ephsert(#expr, "[ASSERT] ", __FILENAME__, __LINE__);
#else
#define ASSERT(expr) \
    switch (0)    \
    case 0:       \
    default:      \
        ;
#endif
