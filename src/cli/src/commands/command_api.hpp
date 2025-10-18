#pragma once

#include <printer/printer.hpp>

#include <iostream>
#include <vector>

class GameContext;

// A simple base class for type erasure.
class ICommandBase {
public:
    virtual ~ICommandBase() = default;

    // The new entry point. It takes the remaining arguments and returns an exit code.
    virtual int run(const std::vector<std::string>& args) = 0;
    virtual void setContext(GameContext* context) { (void)context; }  // Default implementation does nothing.
    virtual void help() = 0;
};

template<typename TArgs, typename TResult, bool TNeedsContext = false>
class Command : public ICommandBase {
public:

    virtual TResult execute(const TArgs& args) = 0;
    virtual TArgs parse(const std::vector<std::string>& args) = 0;

    // Override the base class run method.
    int run(const std::vector<std::string>& args) final
    {
        try {
            // Pre-execution check for commands that require a context.
            if constexpr (TNeedsContext) {
                if (!m_context) {
                    throw std::runtime_error("Command requires a game context, but none was provided.");
                }
            }
            TArgs typed_args = parse(args);
            TResult result = execute(typed_args);
            return 0;  // Success
        }
        catch (const std::exception& e) {
            prnt::err << "Error: " << e.what() << std::endl;
            return 1;  // Failure
        }
    }

    // This override only exists if TNeedsContext is true!
    void setContext(GameContext* context) final
    {
        if constexpr (TNeedsContext) {
            m_context = context;
        }
    }

protected:
    // This member variable is compiled out if TNeedsContext is false.
    // It has zero memory cost for commands that don't need it.
    GameContext* m_context = nullptr;
};

class CommandNoArgs : public ICommandBase {
public:
    virtual void execute() = 0;

    int run(const std::vector<std::string>& args) final
    {
        try {
            if (!args.empty()) {
                throw std::runtime_error("This command does not accept arguments.");
            }
            execute();
            return 0;  // Success
        }
        catch (const std::exception& e) {
            prnt::err << "Error: " << e.what() << std::endl;
            return 1;  // Failure
        }
    }
};
