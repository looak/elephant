#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Forward declare the base interface
class ICommandBase;

// This struct holds all metadata for a registered command.
struct CommandInfo {
    std::string_view name;
    std::string_view description;
    int priority;  // Lower numbers are higher priority
    std::function<std::unique_ptr<ICommandBase>()> factory;

    // A comparison operator to allow sorting
    bool operator<(const CommandInfo& other) const { return priority < other.priority; }
};

class CommandRegistry {
public:    
    static CommandRegistry& instance()
    {
        static CommandRegistry reg;
        return reg;
    }

    // Called by commands to register themselves at startup.
    void registerCommand(CommandInfo info) { m_commands[info.name] = std::move(info); }

    // Creates a new instance of a command by name.
    std::unique_ptr<ICommandBase> createCommand(const std::string& name)
    {
        auto it = m_commands.find(name);
        if (it == m_commands.end()) {
            return nullptr;
        }
        return it->second.factory();
    }

    // Returns a list of all commands, sorted by priority.
    std::vector<CommandInfo> getCommands() const
    {
        std::vector<CommandInfo> commands;
        for (const auto& pair : m_commands) {
            commands.push_back(pair.second);
        }
        std::sort(commands.begin(), commands.end());
        return commands;
    }

private:
    // Private constructor and deleted copy/move to enforce singleton pattern.
    CommandRegistry() = default;
    ~CommandRegistry() = default;
    CommandRegistry(const CommandRegistry&) = delete;
    CommandRegistry& operator=(const CommandRegistry&) = delete;

    std::map<std::string_view, CommandInfo> m_commands;
};

template<typename T>
class CommandRegistrar {
public:
    CommandRegistrar(std::string_view keyword, std::string_view description, int priority)
    {
        CommandRegistry::instance().registerCommand({keyword, description, priority,
                                                     // The factory is a simple lambda that creates a new instance.
                                                     [] { return std::make_unique<T>(); }});
    }
};

#define REG_COMMAND(keyword, type) \
    inline void register_##type() { \
    static CommandRegistrar<type> registrar_##type(keyword, type::description(), type::priority());\
    }