// ElephantGambit.cpp : Defines the entry point for the application.
//
#include "elephant_cli.h"
// #include "commands.h"
// #include "commands_uci.h"
// #include "commands_utils.h"
#include "elephant_cli_config.h"
#include "elephant_gambit_config.h"
#include <core/game_context.hpp>
#include <debug/log.hpp>

#include "commands/command_api.hpp"
#include "command_logic/command_registry.hpp"
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
    MESSAGE() << "                                                                      v: " << ELEPHANT_GAMBIT_VERSION_STR;

#ifdef EG_DEBUGGING
    MESSAGE() << "\nEG_DEBUGGING\n v: ";
    MESSAGE() << ELEPHANT_GAMBIT_VERSION_STR << ELEPHANT_GAMBIT_VERSION_PRERELEASE << ELEPHANT_GAMBIT_VERSION_SUFFIX;
    MESSAGE() << ELEPHANT_CLI_VERSION_STR << ELEPHANT_CLI_VERSION_PRERELEASE << ELEPHANT_CLI_VERSION_SUFFIX;
#endif
}

void
Application::RunUci()
{
#ifdef OUTPUT_LOG_TO_FILE
    LoggingInternals::ScopedDualRedirect redirect_cout(std::cout, LoggingInternals::LogHelpers::readOutputFilename());
#endif
    //UCICommands::UCIEnable();
}

void
Application::Run()
{
#ifdef OUTPUT_LOG_TO_FILE
    LoggingInternals::ScopedDualRedirect redirect_cout(std::cout, LoggingInternals::LogHelpers::readOutputFilename());
#endif

    AppContext context;

    while (1) {
        std::cout << " > ";
        std::string buffer = "";

        if (!std::getline(std::cin, buffer))
            break;

        if (buffer.empty())
            continue;

        if (context.processInput(buffer))
        {
            continue;
        }
        else
        {
            break;
        }
    }
}
