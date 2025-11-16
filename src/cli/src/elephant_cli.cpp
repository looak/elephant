// ElephantGambit.cpp : Defines the entry point for the application.
//
#include "elephant_cli.hpp"
// #include "commands.h"
// #include "commands_uci.h"
// #include "commands_utils.h"
#include "elephant_cli_config.h"
#include "elephant_gambit_config.h"
#include <core/game_context.hpp>
#include <debug/log.hpp>

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"
#include "printer/printer.hpp"

#include <iostream>
#include <list>
#include <sstream>
#include <string>

Application::Application()
{
    MESSAGE() << "           88                        88";
    MESSAGE() << "           88                        88                                   ,d";
    MESSAGE() << "           88                        88                                   88";
    MESSAGE() << " ,adPPYba, 88  ,adPPYba, 8b,dPPYba,  88,dPPYba,  ,adPPYYba, 8b,dPPYba,  MM88MMM";
    MESSAGE() << "a8P_____88 88 a8P_____88 88P'    *8a 88P'    *8a **     `Y8 88P'   `*8a   88";
    MESSAGE() << "8PP******* 88 8PP******* 88       d8 88       88 ,adPPPPP88 88       88   88";
    MESSAGE() << "*8b,   ,aa 88 *8b,   ,aa 88b,   ,a8* 88       88 88,    ,88 88       88   88,";
    MESSAGE() << " `*Ybbd8*' 88  `*Ybbd8*' 88`YbbdP*'  88       88 `*8bbdP*Y8 88       88   *Y888";
    MESSAGE() << "                         88";
    MESSAGE() << "                         88                                               *j*m";
    MESSAGE() << "\n                                                            a uci chess engine";
    MESSAGE() << "                                                                     v: " << ELEPHANT_GAMBIT_VERSION_STR;
    MESSAGE() << "                                                                      " << ELEPHANT_GAMBIT_GIT_HASH;

#ifdef DEVELOPMENT_BUILD
    MESSAGE() << "---------DEVELOPMENT BUILD---------";
    MESSAGE() << " versions numbers:";
    MESSAGE() << " engine: " <<ELEPHANT_GAMBIT_VERSION_STR << ELEPHANT_GAMBIT_VERSION_PRERELEASE << ELEPHANT_GAMBIT_VERSION_SUFFIX;
    MESSAGE() << "    cli: " << ELEPHANT_CLI_VERSION_STR << ELEPHANT_CLI_VERSION_PRERELEASE << ELEPHANT_CLI_VERSION_SUFFIX;
    MESSAGE() << "    git: " << ELEPHANT_GAMBIT_GIT_HASH;
    MESSAGE() << " timestamps:";
    MESSAGE() << "  cmake:  " << ELEPHANT_GAMBIT_BUILD_TIMESTAMP;
    MESSAGE() << "  binary: " << __DATE__ << " at " << __TIME__;
    MESSAGE() << "----------------------------------";
#endif
}

void
Application::RunUci()
{
#ifdef OUTPUT_LOG_TO_FILE
    LoggingInternals::ScopedDualRedirect redirect_cout(std::cout, LoggingInternals::LogHelpers::readOutputFilename());
#endif
    AppContext context;
    context.setState(std::make_unique<UciModeProcessor>());
    context.processInput("");
}

void
Application::Run(char* argv[])
{
#ifdef OUTPUT_LOG_TO_FILE
    LoggingInternals::ScopedDualRedirect redirect_cout(std::cout, LoggingInternals::LogHelpers::readOutputFilename());
#endif
    AppContext context;

    if (argv != nullptr) {
        std::ostringstream oss;
        for (int i = 1; argv[i] != nullptr; ++i) {
            oss << argv[i];
            if (argv[i + 1] != nullptr)
                oss << " ";
        }
        std::string commandLine = oss.str();
        if (context.processInput(commandLine)) {
            return;
        }
    }

    while (1) {

        if (context.handlesInput())
            context.processInput("");
        else {
            std::cout << " > ";
            std::string buffer = "";

            if (!std::getline(std::cin, buffer))
                break;

            if (buffer.empty())
                continue;

            if (context.processInput(buffer)) {
                continue;
            }
            else {
                break;
            }
        }
    }
}
