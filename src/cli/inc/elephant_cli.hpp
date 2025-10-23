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
#include <memory>
#include "commands/logic/command_processor.hpp"

class AppContext {
private:
    std::unique_ptr<ICommandProcessor> m_currentProcessor;

public:
    AppContext() { m_currentProcessor = std::make_unique<NormalModeProcessor>(); }
    ~AppContext() { m_currentProcessor.reset(); }

    void setState(std::unique_ptr<ICommandProcessor> newProcessor) { m_currentProcessor = std::move(newProcessor); }
    bool processInput(const std::string& line) { return m_currentProcessor->processInput(*this, line); }
};

class Application {
public:
    Application();
    ~Application() = default;

    void Run();
    void RunUci();
};