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

//#define OUTPUT_LOG_TO_FILE
// #define EG_DEBUGGING

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

// @brief Logs an error message with the file name, function name and line number.
#define LOG_ERROR() \
    switch (0)      \
    case 0:         \
    default:        \
        LoggingInternals::LogMessage("[    ERROR ] ", __FILENAME__, __FUNCTION__, __LINE__)

// @brief Logs an error message with the file name, function name and line number if the expression evaluates to false.
#define LOG_ERROR_EXPR(expr) \
    if ((expr) != 0) {       \
        int ___noop = 5;     \
        (void)___noop;       \
    }                        \
    else                     \
        LoggingInternals::LogMessage("[    ERROR ] ", __FILENAME__, __FUNCTION__, __LINE__)

#ifdef LOGGING_ENABLED
// @brief Logs an info message with the file name and line number.
#define LOG_INFO() \
    switch (0)     \
    case 0:        \
    default:       \
        LoggingInternals::LogMessage("[     INFO ] ", __FILENAME__, __LINE__)
#else
#define LOG_INFO() LoggingInternals::NopMessage()
#endif

// @brief Logs a debug message with the file name and line number.
#if defined(DEBUG_LOGGING_ENABLED) && defined(LOGGING_ENABLED)
#define LOG_DEBUG() \
    switch (0)      \
    case 0:         \
    default:        \
        LoggingInternals::DebugLogMessage()
#else
#define LOG_DEBUG() LoggingInternals::NopMessage()
#endif

// @brief Logs a warning message with the file name and line number.
#define LOG_WARNING() \
    switch (0)        \
    case 0:           \
    default:          \
        LoggingInternals::LogMessage("[  WARNING ] ", __FILENAME__, __LINE__)

// @brief Logs a warning message with the file name and line number.
#define LOG_WARNING_EXPR(expr) \
    if ((expr) != 0) {    \
        int ___noop = 5;  \
        (void)___noop;    \
    }                     \
    else                  \
        LoggingInternals::LogMessage("[  WARNING ] ", __FILENAME__, __LINE__)

// @brief Asserts that the expression evaluates to true and logs a fatal assert message with the expression, file name and line
// number if it fails.
#if defined(FATAL_ASSERTS_ENABLED)
#define FATAL_ASSERT(expr) \
    if ((expr) != 0) {     \
        int ___noop = 5;   \
        (void)___noop;     \
    }                      \
    else                   \

        LoggingInternals::AssertMessage(#expr, "[FATAL ASRT] ", __FILENAME__, __LINE__)
#else
#define FATAL_ASSERT(expr) LoggingInternals::NopMessage(expr)
#endif

// @brief Logs a basic message without any prefix or suffix.
#define MESSAGE() \
    switch (0)    \
    case 0:       \
    default:      \
        LoggingInternals::BasicMessage()

namespace LoggingInternals {

class LogHelpers {
public:
    static std::string readOutputFilename() { return s_outputFileName; };
    // @brief Generates a unique filename based on time and date.
    static std::string generateUniqueFilename()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t timetNow = std::chrono::system_clock::to_time_t(now);
        std::tm* localtime = std::localtime(&timetNow);

        std::ostringstream filename;
        filename << "output_" << std::put_time(localtime, "%Y%m%d_%H%M%S") << ".log";

        return filename.str();
    }

private:
    static std::string s_outputFileName;
};

typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);

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

/**
 * @brief Scoped Redirect, will redirect given outstream to file & it's original target.  */
class ScopedDualRedirect {
public:
    ScopedDualRedirect(std::ostream& stream, const std::string& filename) :
        m_originalStream(stream),
        m_originalBuffer(stream.rdbuf()),
        m_fileStream(filename, std::ios::app)
    {
        m_dualBuffer = new DualStreamBuffer(m_originalStream.rdbuf(), m_fileStream.rdbuf());
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
    DualStreamBuffer* m_dualBuffer;
};

class MessageStream {
public:
    MessageStream(std::ostream& stream = std::cout) :
        m_stream(stream)        
    {
    }

    ~MessageStream()
    { 
        if (m_needs_endl)
            m_stream << std::endl;
    }

    template<typename T>
    inline MessageStream& operator<<(const T& t)
    {
#ifdef OUTPUT_LOG_TO_FILE
        ScopedDualRedirect redirect_cout(m_stream, LogHelpers::readOutputFilename());
#endif
        std::osyncstream output(m_stream);
        output << t;
        return *this;
    }    

    template<typename T>
    inline MessageStream& operator<<(T* const& pointer)
    {
#ifdef OUTPUT_LOG_TO_FILE
        ScopedDualRedirect redirect_cout(m_stream, LogHelpers::readOutputFilename());
#endif
        std::osyncstream output(m_stream);
        if (pointer == nullptr)
            output << "(nullptr)";
        else
            output << pointer;
        return *this;
    }

    inline MessageStream& operator<<(bool value)
    {
#ifdef OUTPUT_LOG_TO_FILE
        ScopedDualRedirect redirect_cout(m_stream, LogHelpers::readOutputFilename());
#endif
        std::osyncstream output(m_stream);
        output << (value ? "true" : "false");
        return *this;
    }

    void deny_endl() { m_needs_endl = false; }
    std::ostream& stream() { return m_stream; }

private:
    // The type of basic IO manipulators (endl, ends, and flush) for
    // narrow streams.
    bool m_needs_endl = true;
    std::ostream& m_stream;
    void operator=(const MessageStream& other);
};

inline MessageStream& operator<<(MessageStream& out, BasicNarrowIoManip manip)
{
    if (static_cast<BasicNarrowIoManip>(std::endl) == manip)
        out.deny_endl();
    out.stream() << manip;
    return out;
}

// inline MessageStream&& operator<<(MessageStream&& out, BasicNarrowIoManip manip)
// {
//     if (static_cast<BasicNarrowIoManip>(std::endl) == manip)
//         out.deny_endl(); // Prevent double endl in destructor
//     out.stream() << manip;
//     return std::move(out); // Keep it as an rvalue for potential further chaining
// }

inline std::ostream& operator<<(MessageStream&& out, BasicNarrowIoManip manip)
{
    // Apply your custom logic if needed
    if (static_cast<BasicNarrowIoManip>(std::endl) == manip) {
        out.deny_endl(); // Prevent the destructor from adding another endl
    }
    
    // Pass the manipulator to the underlying stream and return the stream itself
    // This allows further chaining with standard manipulators.
    return out.stream() << manip; 
}

class NopMessage {
public:
    constexpr NopMessage(bool ___noop = true) { (void)___noop; }

    template<typename T>
    constexpr inline NopMessage& operator<<(const T&)
    {
        return *this;
    }

    typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);
    constexpr inline NopMessage& operator<<(BasicNarrowIoManip) { return *this; }
};

class LogMessage {
protected:
    std::ostream& m_stream;
    std::unique_ptr<std::string> m_message;
    std::unique_ptr<std::string> m_userMessage;

    void flush()
    {
        std::osyncstream syncOut(m_stream);
        if (m_userMessage.get() != nullptr && !m_userMessage->empty())
            syncOut << m_message->c_str() << " > " << m_userMessage->c_str() << "\n";
        else if (!m_message->empty())
            syncOut << m_message->c_str() << " ";
    }

public:
    LogMessage(std::ostream& stream = std::cout) :
        m_stream(stream),
        m_message(new ::std::string)
    {
    }

    LogMessage(const std::string& prefix, const std::string& file, const std::string& function, int line,
               std::ostream& stream = std::cout) :
        m_stream(stream),
        m_message(new ::std::string)
    {
        m_message->append(prefix)
            .append(file)
            .append(":")
            .append(std::to_string(line))
            .append(" ")
            .append(function)
            .append("()");

        std::osyncstream output(m_stream);
        output << *m_message << " ";
    }

    LogMessage(const std::string& prefix, const std::string& file, int line, std::ostream& stream = std::cout) :
        m_stream(stream),
        m_message(new ::std::string)
    {
        m_message->append(prefix).append(file).append(":").append(std::to_string(line));
        std::osyncstream output(m_stream);
        output << *m_message << " ";
    }

    virtual ~LogMessage()
    {
#ifdef OUTPUT_LOG_TO_FILE
        ScopedDualRedirect redirect_cerr(std::cerr, LogHelpers::readOutputFilename());
#endif

        //flush();

        //std::string* strPtr = m_message.release();
        //if (strPtr != nullptr)
        //    delete strPtr;

        //strPtr = m_userMessage.release();
        //if (strPtr != nullptr)
        //    delete strPtr;
    }

    template<typename T>
    inline MessageStream operator<<(const T& value)
    {
        MessageStream msg;
        msg << value;
        return msg;
    }
};

class DebugLogMessage : public LogMessage {
public:
    DebugLogMessage() :
        LogMessage(std::cerr),
        m_redirect(m_stream, LogHelpers::readOutputFilename())
    {
    }

    DebugLogMessage(const std::string& prefix, const std::string& file, const std::string& function, int line) :
        LogMessage(prefix, file, function, line, std::cerr),
        m_redirect(m_stream, LogHelpers::readOutputFilename())
    {
    }

    DebugLogMessage(const std::string& prefix, const std::string& file, int line) :
        LogMessage(prefix, file, line, std::cerr),
        m_redirect(m_stream, LogHelpers::readOutputFilename())
    {
    }

    ~DebugLogMessage() { flush(); }

private:
    ScopedDualRedirect m_redirect;
};

class BasicMessage {
private:
    std::ostream& m_stream;

public:
    BasicMessage(std::ostream& stream = std::cout) :
        m_stream(stream)
    {}

    template<typename T>
    inline MessageStream operator<<(const T& value)
    {
        MessageStream msg(m_stream);
        msg << value;
        return msg;
    }

        std::ostream& stream() { return m_stream; }
};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4722)  // I'm aborting the application in the ~dtor which compilers don't like.
#endif

class AssertMessage : public LogMessage {
public:
    AssertMessage(char const* expression, const std::string& prefix, const std::string& file, int line) :
        LogMessage(prefix, file, line)
    {
        m_message->append(" expr(").append(expression).append(")");
    }

    ~AssertMessage()
    {
#ifdef OUTPUT_LOG_TO_FILE
        ScopedDualRedirect redirect_cerr(std::cerr, LogHelpers::readOutputFilename());
#endif

        if (m_userMessage.get() != nullptr && !m_userMessage->empty())
            std::cerr << m_message->c_str() << " > " << m_userMessage->c_str() << "\n";
        else if (!m_message->empty())
            std::cerr << m_message->c_str() << "\n";

        std::string* strPtr = m_message.release();
        if (strPtr != nullptr)
            delete strPtr;

        strPtr = m_userMessage.release();
        if (strPtr != nullptr)
            delete strPtr;

        abort();
    }
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}  // namespace LoggingInternals
