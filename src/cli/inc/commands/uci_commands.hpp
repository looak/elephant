// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2023  Alexander Loodin Ek

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

/**
 * @file commands_uci.h
 * @brief UCI interface implementation for the engine.
 * https://www.wbec-ridderkerk.nl/html/UCIProtocol.html
 * @author Alexander Loodin Ek  */
#pragma once
#include <defines.hpp>
#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <string>

class UCI;

namespace UCICommands {

typedef std::function<bool(std::list<std::string>, UCI&)> UCICommandFunction;
typedef std::map<std::string, UCICommandFunction> UCICommandsMap;
typedef std::map<std::string, std::string> UCIOptionsMap;

bool DebugCommand(std::list<std::string> args, UCI& context);
bool IsReadyCommand(std::list<std::string> args, UCI& context);
bool SetOptionCommand(std::list<std::string> args, UCI& context);
bool RegisterCommand(std::list<std::string> args, UCI& context);
bool NewGameCommand(std::list<std::string> args, UCI& context);
bool PositionCommand(std::list<std::string> args, UCI& context);
bool GoCommand(std::list<std::string> args, UCI& context);
bool BenchCommand(std::list<std::string> args, UCI& context);
bool StopCommand(std::list<std::string> args, UCI& context);
bool PonderHitCommand(std::list<std::string> args, UCI& context);
bool PrintCommand(std::list<std::string> args, UCI& context);
bool QuitCommand(std::list<std::string> args, UCI& context);

static UCICommandsMap commands = {
    { "debug", DebugCommand },
    { "isready", IsReadyCommand },
    { "setoption", SetOptionCommand },
    { "register", RegisterCommand },
    { "ucinewgame", NewGameCommand },
    { "position", PositionCommand },
    { "go", GoCommand },
    { "bench", BenchCommand },
    { "stop", StopCommand },
    { "ponderhit", PonderHitCommand },
    { "print", PrintCommand },
    { "quit", QuitCommand }
};

static UCIOptionsMap options = {
    { "Threads", "type spin default 1 min 1 max 24" },
    { "Hash", "type spin default 8 min 1 max 1024"}
};

} // namespace UCICommands


using CommandFunction = std::function<bool(UCI&)>;
struct UCIThread {
private:
    UCI& interface;
    u32 m_id;

    std::mutex m_mtx;
    std::condition_variable_any m_cv;
    std::queue<CommandFunction> m_commandQueue;
    std::vector<std::future<bool>> m_runningCommands;

    void cleanupCompletedFutures();

public:
    UCIThread(UCI& interface, u32 id)
        : interface(interface), m_id(id) {}

    void queue(std::list<std::string> args, UCICommands::UCICommandFunction command);
    void process(std::stop_token stopToken);
    
};