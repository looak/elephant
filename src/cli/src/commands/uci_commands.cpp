#include "commands/uci_commands.hpp"

#include <io/printer.hpp>
#include <move/move.hpp>
#include <core/uci.hpp>

bool UCICommands::DebugCommand(std::list<std::string>, UCI&)
{
    LOG_ERROR() << "Not implemented";
    return false;
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
    std::cout << "bye bye\n";

    return true;
}

void UCIThread::queue(std::list<std::string> args, UCICommands::UCICommandFunction command)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    m_commandQueue.push([args = std::move(args), command] (UCI& _interface) {
        return command(args, _interface);
    });
    m_cv.notify_one();
}

void UCIThread::cleanupCompletedFutures() {    
    m_runningCommands.erase(
        std::remove_if(m_runningCommands.begin(), m_runningCommands.end(),
            [](std::future<bool>& fut) {
                if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    try {
                        bool result = fut.get();
                        if (!result) {
                            LOG_ERROR() << "A command failed to execute properly.";
                        }
                    } catch (const std::exception& e) {
                        LOG_ERROR() << "Command threw exception: " << e.what();
                    }
                    return true; // Remove this future
                }
                return false; // Keep this future
            }
        ),
        m_runningCommands.end()
    );
}

void UCIThread::process(std::stop_token stopToken)
{
    while (!stopToken.stop_requested()) {
        cleanupCompletedFutures();

        CommandFunction command;
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            if (m_cv.wait(lock, stopToken, [this]() { return !m_commandQueue.empty(); }) == false)
                break;
                
            command = m_commandQueue.front();
            m_commandQueue.pop();
        }

        m_runningCommands.push_back(std::async(std::launch::async, command, std::ref(interface)));
    }
}