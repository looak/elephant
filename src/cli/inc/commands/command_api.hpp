#pragma once

#include <core/game_context.hpp>
#include <position/position_accessors.hpp>
#include <printer/printer.hpp>

#include <iostream>
#include <vector>

// A simple base class for type erasure.
class ICommandBase {
public:
    virtual ~ICommandBase() = default;

    // The new entry point. It takes the remaining arguments and returns an exit code.
    virtual int run(const std::vector<std::string>& args) = 0;
    virtual void setContext(GameContext* context) { (void)context; }  // Default implementation does nothing.
    virtual void help(bool extended = false) = 0;
};

template<typename TArgs, bool TNeedsContext = false>
class Command : public ICommandBase {
public:
    virtual bool execute(const TArgs& args) = 0;
    virtual std::optional<TArgs> parse(const std::vector<std::string>& args) = 0;

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
            std::optional<TArgs> typed_args = parse(args);
            if (typed_args.has_value() == false) {
                //prnt::err << " Error: Failed to parse command arguments." << std::endl;
                return 1;  // Parsing failed
            }

            return execute(typed_args.value()) ? 0 : 1;  // Return 0 on success, 1 on failure
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

template<typename TArgs>
class ReadOnlyCommand : public ICommandBase {
public:
    virtual bool execute(const TArgs& args) = 0;
    virtual std::optional<TArgs> parse(const std::vector<std::string>& args) = 0;

    int run(const std::vector<std::string>& args) final
    {
        try {
            if (!args.empty())
                throw std::runtime_error("This command does not accept arguments.");

            if (m_context == nullptr) 
                    throw std::runtime_error("Command requires a game context, but none was provided.");

            std::optional<TArgs> typed_args = parse(args);
            if (typed_args.has_value() == false) {
                prnt::err << "Error: Failed to parse command arguments." << std::endl;
                return 1;  // Parsing failed
            }

            return execute(typed_args.value()) ? 0 : 1;  // Return 0 on success, 1 on failure
        }
        catch (const std::exception& e) {
            prnt::err << "Error: " << e.what() << std::endl;
            return 1;  // Failure
        }

        return 0;  // Success
    }

    void setContext(GameContext* context) final
    {
        m_context = context;
    }

protected:
    PositionReader readPosition() const { return m_context->readChessPosition(); }

private:
    GameContext* m_context = nullptr;
};

template<bool TNeedsContext = false>
class CommandNoArgs : public ICommandBase {
public:
    virtual bool execute() = 0;

    int run(const std::vector<std::string>& args) final
    {
        try {
            if constexpr (TNeedsContext) {
                if (m_context == nullptr) 
                    throw std::runtime_error("Command requires a game context, but none was provided.");
            }
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
    void setContext(GameContext* context) final
    {
        m_context = context;
    }
protected:
    GameContext* m_context = nullptr;    
};
