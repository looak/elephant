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

// The log, the console output and, with OUTPUT_LOG_TO_FILE, a file. Include it where something is logged: the sinks
// and their setup live in logger.cpp, this only brings in spdlog's logging calls.
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

#define LOG_TRACE(...)    SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

namespace logging {

std::string readLogFilename();
std::string readCoutFilename();

// the console logger at LOG_LEVEL, with OUTPUT_LOG_TO_FILE also a rotating file in logs/.
void init();
void deinit();

#ifdef DEBUG_SEARCH_TREE
std::shared_ptr<spdlog::logger> debug_search_logger();
#endif
} // namespace logging
