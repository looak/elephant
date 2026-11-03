// ICommandProcessor.hpp
#pragma once
#include <string>
#include <memory>

#include "commands/command_context.hpp"

class AppContext;

// Interface for any object that can process user input.
class ICommandProcessor {
public:
    virtual bool handlesInput() { return false; }
    virtual ~ICommandProcessor() = default;
    // The main entry point. Returns false if the app should exit.
    virtual bool processInput(AppContext* context, const std::string& line) = 0;
};

class NormalModeProcessor : public ICommandProcessor {
private:
    CommandContext m_context;

public:
    bool processInput(AppContext* context, const std::string& line) override;
};

class UciModeProcessor : public ICommandProcessor {
public:
    bool handlesInput() override { return true; }
    bool processInput(AppContext* context, const std::string& line) override;

};