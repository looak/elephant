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
#define LOG_INFO() Log::LogInfoImpl(__FILENAME__, __LINE__)
#define LOG_WARNING() Log::LogWarningImpl(__FILENAME__, __LINE__)
#define FATAL_ASSERT(expr) Log::AssertImpl(expr, __FILENAME__, __FUNCTION__, __LINE__)

class LogMessage
{
public:
	LogMessage& operator<<(const char* stream)
	{
		if (stream != nullptr)
			std::cout << " > message: " << stream;
		std::cout << std::endl;
		return *this;
	}
};

class Log
{
public:
	static LogMessage LogInfoImpl(const char* file, const int line)
	{
		std::cout << "[      INFO] " << file << ":" << line;
		return LogMessage();
	}

	static LogMessage LogWarningImpl(const char* file, const int line)
	{
		std::cout << "[   WARNING] " << file << ":" << line;
		return LogMessage();
	}

	static LogMessage LogErrorImpl(const char* file, const char* function, const int line)
	{
		std::cout << "[     ERROR] " << file << ":" << line << " > " << function;
		return LogMessage();
	}

	static LogMessage AssertImpl(int expression, const char* file, const char* function, const int line)
	{
		if (expression == 0)
		{
			std::cout << "[FATAL ASRT] " << file << ":" << line << " > " << function;
			assert(expression);
		}
		return LogMessage();
	}
};