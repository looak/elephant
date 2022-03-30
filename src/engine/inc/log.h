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

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define LOG_ERROR() Log::LogErrorImpl(__FILENAME__, __FUNCTION__, __LINE__)
#define LOG_ERROR_EXPR(expr) Log::LogErrorImpl(expr, __FILENAME__, __FUNCTION__, __LINE__)
#define LOG_INFO() Log::LogInfoImpl(__FILENAME__, __LINE__)
#define LOG_WARNING() Log::LogWarningImpl(__FILENAME__, __LINE__)
#define FATAL_ASSERT(expr) Log::AssertImpl(expr, __FILENAME__, __FUNCTION__, __LINE__)


class BaseMessage
{
public:
	virtual BaseMessage& operator<<(const char* stream)
	{
		return *this;
	}

	virtual BaseMessage& operator<<(const char stream)
	{	
		return *this;
	}
	
	virtual BaseMessage& operator<<(int value)
	{		
		return *this;
	}
};
class LogMessage : public BaseMessage
{
public:
	BaseMessage& operator<<(const char* stream) override
	{
		if (stream != nullptr)
			std::cout << " > message: " << stream;
		std::cout << std::endl;
		return *this;
	}

	BaseMessage& operator<<(const char stream) override
	{		
		std::cout << " > message: " << stream;
		std::cout << std::endl;
		return *this;
	}
	
	BaseMessage& operator<<(int value) override
	{		
		std::cout << " > message: " << value;
		std::cout << std::endl;
		return *this;
	}
};

class EmptyMessage : public BaseMessage
{
public:
	BaseMessage& operator<<(const char* stream) override
	{
		return *this;
	}

	BaseMessage& operator<<(const char stream) override
	{
		return *this;
	}
	
	BaseMessage& operator<<(int value) override
	{
		return *this;
	}
};

class Log
{
public:
	static BaseMessage& LogInfoImpl(const char* file, const int line)
	{
		std::cout << "[      INFO] " << file << ":" << line;
		return s_logMessage;
	}

	static BaseMessage& LogWarningImpl(const char* file, const int line)
	{
		std::cout << "[   WARNING] " << file << ":" << line;
		return s_logMessage;
	}
	
	static BaseMessage& LogErrorImpl(int expression, const char* file, const char* function, const int line)
	{
		if (expression == 0)
		{
			return LogErrorImpl(file, function, line);
		}

		return s_emptyMessage;
	}

	static BaseMessage& LogErrorImpl(const char* file, const char* function, const int line)
	{
		std::cout << "[     ERROR] " << file << ":" << line << " > " << function;
		return s_logMessage;
	}

	static BaseMessage& AssertImpl(int expression, const char* file, const char* function, const int line)
	{
		if (expression == 0)
		{
			std::cout << "[FATAL ASRT] " << file << ":" << line << " > " << function;
			assert(expression);
		}
		return s_emptyMessage;
	}

private:
	static EmptyMessage s_emptyMessage;
	static LogMessage s_logMessage;
};