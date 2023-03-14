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

#define LOG_ERROR() \
switch(0) case 0: default: LogMessage("[    ERROR ] ", __FILENAME__, __FUNCTION__, __LINE__)

#define LOG_ERROR_EXPR(expr) \
switch(0) case 0: default: if(expr != 0){ int ___noop = 5; (void)___noop;} else LogMessage("[    ERROR ] ", __FILENAME__, __FUNCTION__, __LINE__)

#define LOG_INFO() \
switch(0) case 0: default: LogMessage("[     INFO ] ", __FILENAME__, __LINE__)
#define LOG_WARNING() \
switch(0) case 0: default: LogMessage("[  WARNING ] ", __FILENAME__, __LINE__)
#define FATAL_ASSERT(expr) \
switch(0) case 0: default: if(expr != 0){ int ___noop = 5; (void)___noop;} else AssertMessage(#expr, "[FATAL ASRT] ", __FILENAME__, __LINE__)

#define MESSAGE() \
switch(0) case 0: default: LogMessage()

class BaseMessage
{
private:
  	// The type of basic IO manipulators (endl, ends, and flush) for
  	// narrow streams.
  	typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);

	const std::unique_ptr< ::std::stringstream> m_stream;

	void operator=(const BaseMessage& other);

public:
	BaseMessage() :
	  m_stream(new ::std::stringstream)
	{}

	BaseMessage(const BaseMessage& msg) :
	  m_stream(new ::std::stringstream)
	{
		*m_stream << msg.getString();		
	}

	std::string getString() const
	{
		return m_stream->str();
	}

	template<typename T>
	BaseMessage& operator<<(const T& t)
	{
		*m_stream << t;
		return *this;
	}

	template<typename T>
	inline BaseMessage& operator<<(T* const& pointer)
	{
		if(pointer == nullptr)
			*m_stream << "(nullptr)";
		else
			*m_stream << pointer;

		return *this;
	}

	template<typename T>
	inline BaseMessage& operator<<(BasicNarrowIoManip value)
	{
		*m_stream << value;
		return *this;
	}

	// bool implementation
	inline BaseMessage& operator<<(bool value)
	{
		*m_stream << (value ? "true" : "false");
		return *this;
	}
};

class LogMessage
{	
private:
	void AppendMessage(const BaseMessage& message)
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
	}

	template <typename T>
	inline LogMessage& operator<<(const T& value)
	{
		AppendMessage(BaseMessage() << value);
		return *this;
	}

	typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);
	inline LogMessage& operator<<(BasicNarrowIoManip value)
	{
		AppendMessage(BaseMessage() << value);
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

		abort();
	}

};