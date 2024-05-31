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
#include <functional>
#include <map>
#include <string>
#include <list>

class UCI;

namespace UCICommands
{

typedef std::function<bool(std::list<std::string>&, UCI&)> UCICommandFunction;
typedef std::map<std::string, UCICommandFunction> UCICommandsMap;
typedef std::map<std::string, std::string> UCIOptionsMap;

void UCIEnable();

bool DebugCommand(std::list<std::string>& args, UCI& context);
bool IsReadyCommand(std::list<std::string>& args, UCI& context);
bool SetOptionCommand(std::list<std::string>& args, UCI& context);
bool RegisterCommand(std::list<std::string>& args, UCI& context);
bool NewGameCommand(std::list<std::string>& args, UCI& context);
bool PositionCommand(std::list<std::string>& args, UCI& context);
bool GoCommand(std::list<std::string>& args, UCI& context);
bool StopCommand(std::list<std::string>& args, UCI& context);
bool PonderHitCommand(std::list<std::string>& args, UCI& context);
bool QuitCommand(std::list<std::string>& args, UCI& context);

// engine options
// bool DebugOutputOption(std::list<std::string>& args, UCI& context);
// bool ThreadsOption(std::list<std::string>& args, UCI& context);
// bool HashOption(std::list<std::string>& args, UCI& context);
bool ForwardOption(std::list<std::string>& args, UCI& context);

static UCICommandsMap commands = {
    { "debug", DebugCommand },
    { "isready", IsReadyCommand },
    { "setoption", SetOptionCommand },
    { "register", RegisterCommand },
    { "ucinewgame", NewGameCommand },
    { "position", PositionCommand },
    { "go", GoCommand },
    { "stop", StopCommand },
    { "ponderhit", PonderHitCommand },
    { "quit", QuitCommand }
};

static UCIOptionsMap options = {
    { "Threads", "type spin default 1 min 1 max 1." },
    { "Hash", "type is spin size of hash table in megabytes."}
};

} // namespace UCICommands