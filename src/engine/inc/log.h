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
#include <cassert>
#include <cstring>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <time.h>

// #define OUTPUT_LOG_TO_FILE
// #define EG_DEBUGGING

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

// @brief Logs an error message with the file name, function name and line number.
#define LOG_ERROR() \
switch(0) case 0: default: LoggingInternals::LogMessage("[    ERROR ] ", __FILENAME__, __FUNCTION__, __LINE__)

// @brief Logs an error message with the file name, function name and line number if the expression evaluates to false.
#define LOG_ERROR_EXPR(expr) \
if((expr) != 0){ int ___noop = 5; (void)___noop;} else LoggingInternals::LogMessage("[    ERROR ] ", __FILENAME__, __FUNCTION__, __LINE__)

// @brief Logs an info message with the file name and line number.
#define LOG_INFO() \
switch(0) case 0: default: LoggingInternals::LogMessage("[     INFO ] ", __FILENAME__, __LINE__)

// @brief Logs a debug message with the file name and line number.
#ifdef EG_DEBUGGING
#define LOG_DEBUG() \
switch(0) case 0: default: LoggingInternals::DebugLogMessage("[    DEBUG ] ", __FILENAME__, __LINE__)
#else
#define LOG_DEBUG() LoggingInternals::NopMessage()
#endif

// @brief Logs a warning message with the file name and line number.
#define LOG_WARNING() \
switch(0) case 0: default: LoggingInternals::LogMessage("[  WARNING ] ", __FILENAME__, __LINE__)

// @brief Asserts that the expression evaluates to true and logs a fatal assert message with the expression, file name and line number if it fails.
#define FATAL_ASSERT(expr) \
if((expr) != 0){ int ___noop = 5; (void)___noop;} else LoggingInternals::AssertMessage(#expr, "[FATAL ASRT] ", __FILENAME__, __LINE__)

// @brief Logs a basic message without any prefix or suffix.
#define MESSAGE() \
switch(0) case 0: default: LoggingInternals::BasicMessage()

namespace LoggingInternals
{

class LogHelpers
{
public:
    static std::string readOutputFilename() { return s_outputFileName; };
    // @brief Generates a unique filename based on time and date.
    static std::string generateUniqueFilename()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t timetNow = std::chrono::system_clock::to_time_t(now);        
        std::tm* localtime = std::localtime(&timetNow);

        std::ostringstream filename;
        filename << "output_"
                << std::put_time(localtime, "%Y%m%d_%H%M%S")
                << ".log";

        return filename.str();
    }

private:
    static std::string s_outputFileName;
    
};

typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);

class DualStreamBuffer : public std::streambuf 
{
public:
    DualStreamBuffer(std::streambuf* one, std::streambuf* two) :
        m_bufferOne(one),
        m_bufferTwo(two) 
    {}

protected:
    int sync() override 
    {
        int result1 = m_bufferOne->pubsync();
        int result2 = m_bufferTwo->pubsync();
        return (result1 == 0 && result2 == 0) ? 0 : -1;
    }

    std::streamsize xsputn(const char* s, std::streamsize n) override 
    {
        m_bufferOne->sputn(s, n);
        m_bufferTwo->sputn(s, n);
        return n;
    }

    int overflow(int c) override 
    {
        if (c != EOF) 
        {
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
class ScopedDualRedirect
{
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

class MessageStream
{
public:
	MessageStream() :
	  m_stream(new ::std::stringstream)
	{ }

	MessageStream(const MessageStream& msg) :
	  m_stream(new ::std::stringstream)
	{
		*m_stream << msg.getString();		
	}

	~MessageStream()
	{
		m_stream->flush();
		std::stringstream* ssptr = m_stream.release();
		if(ssptr != nullptr)		
			delete ssptr;
	}

	std::string getString() const
	{
		return m_stream->str();
	}

	template<typename T>
	MessageStream& operator<<(const T& t)
	{
		*m_stream << t;
		return *this;
	}

	template<typename T>
	inline MessageStream& operator<<(T* const& pointer)
	{
		if(pointer == nullptr)
			*m_stream << "(nullptr)";
		else
			*m_stream << pointer;

		return *this;
	}

	template<typename T>
	inline MessageStream& operator<<(BasicNarrowIoManip value)
	{
		*m_stream << value;
		return *this;
	}

	// bool implementation
	inline MessageStream& operator<<(bool value)
	{
		*m_stream << (value ? "true" : "false");
		return *this;
	}
    private:
  	// The type of basic IO manipulators (endl, ends, and flush) for
  	// narrow streams.
  	typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);
	std::unique_ptr< ::std::stringstream> m_stream;
	void operator=(const MessageStream& other);

};

class NopMessage
{
public:
    NopMessage() {}

    template <typename T>
	inline NopMessage& operator<<(const T&)
	{		
		return *this;
	}

	typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);
	inline NopMessage& operator<<(BasicNarrowIoManip)
	{		
		return *this;
	}
};

class LogMessage
{	
private:
	void AppendMessage(const MessageStream& message)
	{
		if (m_userMessage.get() == nullptr) 
            m_userMessage.reset(new ::std::string);

		m_userMessage->append(message.getString().c_str());
	}
    
protected:
	std::unique_ptr<std::string> m_message;
	std::unique_ptr<std::string> m_userMessage;

    void flush()
    {
		if (m_userMessage.get() != nullptr && !m_userMessage->empty())
			std::cerr << m_message->c_str() << " > " << m_userMessage->c_str() << "\n";
		else if(!m_message->empty())
			std::cerr << m_message->c_str() << "\n";

        m_userMessage->clear();
        m_message->clear();
    }

public:
	LogMessage() :
	  m_message(new ::std::string)
	{}

	LogMessage(const std::string& prefix, const std::string& file, const std::string& function, int line) :
	  m_message(new ::std::string)
	{
		m_message->append(prefix).append(file).append(":").append(std::to_string(line)).append(" ").append(function).append("()");	
	}

	LogMessage(const std::string& prefix, const std::string& file, int line) :
	  m_message(new ::std::string)
	{
		m_message->append(prefix).append(file).append(":").append(std::to_string(line));
	}

	virtual ~LogMessage()
	{
        #ifdef OUTPUT_LOG_TO_FILE
        ScopedDualRedirect redirect_cerr(std::cerr, LogHelpers::readOutputFilename());
        #endif

        flush();

		std::string* strPtr = m_message.release();
		if (strPtr != nullptr)
			delete strPtr;
		
		strPtr = m_userMessage.release();
		if (strPtr != nullptr)
			delete strPtr;
	}

	template <typename T>
	inline LogMessage& operator<<(const T& value)
	{
		AppendMessage(MessageStream() << value);
		return *this;
	}

	typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);
	inline LogMessage& operator<<(BasicNarrowIoManip value)
	{
		AppendMessage(MessageStream() << value);
		return *this;
	}
};

class DebugLogMessage : public LogMessage
{
public:
	DebugLogMessage() :
      LogMessage(),
      m_redirect(std::cerr, LogHelpers::readOutputFilename())
	{ }

	DebugLogMessage(const std::string& prefix, const std::string& file, const std::string& function, int line) :
        LogMessage(prefix, file, function, line),
        m_redirect(std::cerr, LogHelpers::readOutputFilename())
	{ }

	DebugLogMessage(const std::string& prefix, const std::string& file, int line) :
	  LogMessage(prefix, file, line),
      m_redirect(std::cerr, LogHelpers::readOutputFilename())
	{ }

    ~DebugLogMessage()
    {
        flush();
    }

private:
    ScopedDualRedirect m_redirect;

};

class BasicMessage
{
private:
	void AppendMessage(const MessageStream& message)
	{
		if (m_message.get() == nullptr) m_message.reset(new ::std::string);

		m_message->append(message.getString().c_str());
	}

	std::unique_ptr<std::string> m_message;

public:
	BasicMessage()
	{}

	~BasicMessage()
	{
        #ifdef OUTPUT_LOG_TO_FILE
        ScopedDualRedirect redirect_cout(std::cout, LogHelpers::readOutputFilename());
        #endif

		if (m_message.get() != nullptr)
			std::cout << m_message->c_str() << "\n";

		std::string* strPtr = m_message.release();
		if (strPtr != nullptr)
			delete strPtr;		
	}

	template <typename T>
	inline BasicMessage& operator<<(const T& value)
	{
		AppendMessage(MessageStream() << value);
		return *this;
	}

	inline BasicMessage& operator<<(BasicNarrowIoManip value)
	{
		AppendMessage(MessageStream() << value);
		return *this;
	}

};
#pragma warning(push)
#pragma warning(disable: 4722) // I'm aborting the application in the ~dtor which compilers don't like.

class AssertMessage : public LogMessage
{
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
		else if(!m_message->empty())
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

#pragma warning(pop)

} // namespace LoggingInternals
