#include "commands/uci_commands.hpp"

#include <io/printer.hpp>
#include <move/move.hpp>
#include <core/uci.hpp>

bool UCICommands::DebugCommand(std::list<std::string>, UCI&)
{
    return true;
}

bool UCICommands::IsReadyCommand(std::list<std::string>, UCI& interface)
{
    return interface.IsReady();
}

bool UCICommands::SetOptionCommand(std::list<std::string> args, UCI& interface)
{
    interface.SetOption(args);
    return true;
}

bool UCICommands::RegisterCommand(std::list<std::string>, UCI&)
{
    throw new ephant::not_implemented_exception("UCI Register command not implemented");
}

bool UCICommands::NewGameCommand(std::list<std::string>, UCI& interface) {
    return interface.NewGame();
}

bool UCICommands::PositionCommand(std::list<std::string> args, UCI& interface) {   
    if (!interface.Position(args))
        throw new ephant::io_error("Failed to set position in UCI Position command");

    return true;
}
bool UCICommands::GoCommand(std::list<std::string> args, UCI& interface) {
    interface.Go(args);
    return true;
}
bool UCICommands::BenchCommand(std::list<std::string> args, UCI& interface) {
    return interface.Bench(args);
}
bool UCICommands::StopCommand(std::list<std::string>, UCI& interface) {
    return interface.Stop();
}
bool UCICommands::PonderHitCommand(std::list<std::string>, UCI&) {
    throw new ephant::not_implemented_exception("UCI PonderHit command not implemented");
}

bool UCICommands::PrintCommand(std::list<std::string>, UCI& interface)
{
    io::printer::board(std::cout, interface.readGameContext().readChessboard());
    return true;
}

bool UCICommands::QuitCommand(std::list<std::string>, UCI& interface) {
    std::cout << "bye bye";
    return true;
}

void UCIThreadContext::queue(std::list<std::string> args, UCICommands::UCICommandFunction command)
{    
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_commandQueue.push([args = std::move(args), command] (UCI& _interface) {
            LOG_TRACE("executing command {}", args.front());
            return command(args, _interface);
        });    
    }
    m_cv.notify_one();
}

void UCIThreadContext::process(std::stop_token stopToken) {
    while (!stopToken.stop_requested()) {
        LOG_TRACE("waiting for new command...");
        CommandFunction command;
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            LOG_TRACE("queue size before wait: {}", m_commandQueue.size());
            // wait
            m_cv.wait(lock, [this, &stopToken]()  { 
                return !m_commandQueue.empty() || stopToken.stop_requested();
            });

            LOG_TRACE("woke up, queue size: {}", m_commandQueue.size());
            LOG_TRACE("stop requested: {}", stopToken.stop_requested());
            if (stopToken.stop_requested() && m_commandQueue.empty())
                break;
                
            if (!m_commandQueue.empty()) {
                LOG_TRACE("about to extract command");
                command = std::move(m_commandQueue.front());
                LOG_TRACE("command moved");
                m_commandQueue.pop();
                LOG_TRACE("command popped, queue size now: {}", m_commandQueue.size());
            }
        }

        LOG_TRACE("lock released, command valid: {}", (command ? "yes" : "no"));
        if (command) {
            LOG_TRACE("about to execute command");
            try {
            if (command(interface) == false)       
                LOG_ERROR("UCI Command failed");
            } catch (const std::exception& e) {
                LOG_ERROR("Exception during UCI command execution: {}", e.what());
            }

            LOG_TRACE("command execute done...");
        }
    }
    LOG_TRACE("worker thread exiting");
}