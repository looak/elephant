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
#include <iostream>
#include <cassert>
#include <string>
#include <cstring>
#include <sstream>

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

// @brief Logs an error message with the file name, function name and line number.
#define LOG_ERROR() \
switch(0) case 0: default: LoggingInternals::LogMessage("[    ERROR ] ", __FILENAME__, __FUNCTION__, __LINE__)

// @brief Logs an error message with the file name, function name and line number if the expression evaluates to false.
#define LOG_ERROR_EXPR(expr) \
if(expr != 0){ int ___noop = 5; (void)___noop;} else LoggingInternals::LogMessage("[    ERROR ] ", __FILENAME__, __FUNCTION__, __LINE__)

// @brief Logs an info message with the file name and line number.
#define LOG_INFO() \
switch(0) case 0: default: LoggingInternals::LogMessage("[     INFO ] ", __FILENAME__, __LINE__)

// @brief Logs a warning message with the file name and line number.
#define LOG_WARNING() \
switch(0) case 0: default: LoggingInternals::LogMessage("[  WARNING ] ", __FILENAME__, __LINE__)

// @brief Asserts that the expression evaluates to true and logs a fatal assert message with the expression, file name and line number if it fails.
#define FATAL_ASSERT(expr) \
if(expr != 0){ int ___noop = 5; (void)___noop;} else LoggingInternals::AssertMessage(#expr, "[FATAL ASRT] ", __FILENAME__, __LINE__)

// @brief Logs a basic message without any prefix or suffix.
#define MESSAGE() \
switch(0) case 0: default: LoggingInternals::BasicMessage()

namespace LoggingInternals
{

typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);

class MessageStream
{
private:
  	// The type of basic IO manipulators (endl, ends, and flush) for
  	// narrow streams.
  	typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);

	const std::unique_ptr< ::std::stringstream> m_stream;

	void operator=(const MessageStream& other);

public:
	MessageStream() :
	  m_stream(new ::std::stringstream)
	{}

	MessageStream(const MessageStream& msg) :
	  m_stream(new ::std::stringstream)
	{
		*m_stream << msg.getString();		
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
};

class LogMessage
{	
private:
	void AppendMessage(const MessageStream& message)
	{
		if (m_userMessage.get() == nullptr) m_userMessage.reset(new ::std::string);

		m_userMessage->append(message.getString().c_str());
	}
protected:
	std::unique_ptr<std::string> m_message;
	std::unique_ptr<std::string> m_userMessage;

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
		if (m_userMessage.get() != nullptr)
			std::cerr << m_message->c_str() << " > " << m_userMessage->c_str() << "\n";
		else
			std::cerr << m_message->c_str() << std::endl;
		
		m_message.reset();
		m_userMessage.reset();
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
		if (m_message.get() != nullptr)
			std::cout << m_message->c_str() << "\n";

		m_message.reset();
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
		if (m_userMessage.get() != nullptr)
			std::cerr << m_message->c_str() << " > " << m_userMessage->c_str() << "\n";
		else
			std::cerr << m_message->c_str() << "\n";

		m_message.reset();
		m_userMessage.reset();

		abort();
	}
};

}; // namespace LoggingInternals