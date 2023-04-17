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
 * @author Alexander Loodin Ek
 */
#pragma once
#include <functional>
#include <map>
#include <string>
#include <list>

class GameContext;

namespace UCI
{

typedef std::function<void(std::list<std::string>&, GameContext&)> UCICommandFunction;
typedef std::map<std::string, UCICommandFunction> UCICommandsMap;

void UCIEnable(GameContext& context);

void DebugCommand(std::list<std::string>& args, GameContext& context);
void IsReadyCommand(std::list<std::string>& args, GameContext& context);
void SetOptionCommand(std::list<std::string>& args, GameContext& context);
void RegisterCommand(std::list<std::string>& args, GameContext& context);
void NewGameCommand(std::list<std::string>& args, GameContext& context);
void PositionCommand(std::list<std::string>& args, GameContext& context);
void GoCommand(std::list<std::string>& args, GameContext& context);
void StopCommand(std::list<std::string>& args, GameContext& context);
void PonderHitCommand(std::list<std::string>& args, GameContext& context);
void QuitCommand(std::list<std::string>& args, GameContext& context);

static UCICommandsMap options = {
    { "debug", DebugCommand },
    { "isready", IsReadyCommand },
    { "setoption", SetOptionCommand },
    { "register", RegisterCommand },
    { "ucinewgame", NewGameCommand },
    {"position", PositionCommand },
    {"go", GoCommand },
    { "stop", StopCommand },
    { "ponderhit", PonderHitCommand },
    { "quit", QuitCommand }
};

} // namespace UCI