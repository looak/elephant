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
    LOG_ERROR() << "Not implemented";
    return false;
}

bool UCICommands::NewGameCommand(std::list<std::string>, UCI& interface)
{
    return interface.NewGame();
}

bool UCICommands::PositionCommand(std::list<std::string> args, UCI& interface)
{   
    if (!interface.Position(args))
    {
        LOG_ERROR() << " Something went wrong during position command";
        return false;
    }

    return true;
}
bool UCICommands::GoCommand(std::list<std::string> args, UCI& interface)
{
    interface.Go(args);
    return true;
}
bool UCICommands::BenchCommand(std::list<std::string> args, UCI& interface)
{
    return interface.Bench(args);
}
bool UCICommands::StopCommand(std::list<std::string>, UCI& interface)
{
    return interface.Stop();
}
bool UCICommands::PonderHitCommand(std::list<std::string>, UCI&)
{
    LOG_ERROR() << "Not implemented";
    return false;
}

bool UCICommands::PrintCommand(std::list<std::string>, UCI& interface)
{
    io::printer::board(std::cout, interface.readGameContext().readChessboard());
    return true;
}

bool UCICommands::QuitCommand(std::list<std::string>, UCI& interface)
{
    std::cout << "bye bye";

    return true;
}

void UCIThreadContext::queue(std::list<std::string> args, UCICommands::UCICommandFunction command)
{    
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_commandQueue.push([args = std::move(args), command] (UCI& _interface) {
            LOG_DEBUG() << "executing command " << args.front();
            return command(args, _interface);
        });    
    }
    m_cv.notify_one();
}

void UCIThreadContext::process(std::stop_token stopToken) {
    while (!stopToken.stop_requested()) {
        LOG_DEBUG() << "waiting for new command...";
        CommandFunction command;
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            LOG_DEBUG() << "queue size before wait: " << m_commandQueue.size();
            // wait
            m_cv.wait(lock, [this, &stopToken]()  { 
                return !m_commandQueue.empty() || stopToken.stop_requested();
            });

            LOG_DEBUG() << "woke up, queue size: " << m_commandQueue.size() 
                        << ", stop requested: " << stopToken.stop_requested();

            if (stopToken.stop_requested() && m_commandQueue.empty())
                break;
                
            if (!m_commandQueue.empty()) {
                LOG_DEBUG() << "about to extract command";
                command = std::move(m_commandQueue.front());
                LOG_DEBUG() << "command moved";
                m_commandQueue.pop();
                LOG_DEBUG() << "command popped, queue size now: " << m_commandQueue.size();
            }
        }

        LOG_DEBUG() << "lock released, command valid: " << (command ? "yes" : "no");

        if (command) {
            LOG_DEBUG() << "about to execute command";
            if (command(interface) == false)       
                LOG_ERROR() << "UCI Command failed";        

            LOG_DEBUG() << "command execute done...";
        }
    }
    LOG_DEBUG() << "worker thread exiting";
}