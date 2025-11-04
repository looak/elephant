// ICommandProcessor.hpp
#pragma once
#include <string>
#include <core/uci.hpp>
#include <memory>

#include <core/game_context.hpp>

class AppContext;

// Interface for any object that can process user input.
class ICommandProcessor {
public:
    virtual bool handlesInput() { return false; }
    virtual ~ICommandProcessor() = default;
    // The main entry point. Returns false if the app should exit.
    virtual bool processInput(AppContext& context, const std::string& line) = 0;
};

class NormalModeProcessor : public ICommandProcessor {
private:
    GameContext m_gameContext;  // Local game context for normal mode

public:
    bool processInput(AppContext& context, const std::string& line) override;
};

class UciModeProcessor : public ICommandProcessor {
public:
    bool handlesInput() override { return true; }
    UciModeProcessor();
    bool processInput(AppContext& context, const std::string& line) override;

private:
    void options();
    void extractArgsFromCommand(const std::string& buffer, std::list<std::string>& tokens);   
    
};