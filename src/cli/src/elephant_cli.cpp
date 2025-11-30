// ElephantGambit.cpp : Defines the entry point for the application.
//
#include "elephant_cli.hpp"
#include "elephant_cli_config.h"
#include "elephant_gambit_config.h"
#include <core/game_context.hpp>
#include <diagnostics/logger.hpp>

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"
#include "printer/printer.hpp"

#include <spdlog/spdlog.h>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

Application::Application() { 
    
    prnt::out << " "<< std::endl;
    prnt::out << "           88                        88" << std::endl;
    prnt::out << "           88                        88                                   ,d" << std::endl;
    prnt::out << "           88                        88                                   88" << std::endl;
    prnt::out << " ,adPPYba, 88  ,adPPYba, 8b,dPPYba,  88,dPPYba,  ,adPPYYba, 8b,dPPYba,  MM88MMM" << std::endl;
    prnt::out << "a8P_____88 88 a8P_____88 88P'    *8a 88P'    *8a **     `Y8 88P'   `*8a   88" << std::endl;
    prnt::out << "8PP******* 88 8PP******* 88       d8 88       88 ,adPPPPP88 88       88   88" << std::endl;
    prnt::out << "*8b,   ,aa 88 *8b,   ,aa 88b,   ,a8* 88       88 88,    ,88 88       88   88," << std::endl;
    prnt::out << " `*Ybbd8*' 88  `*Ybbd8*' 88`YbbdP*'  88       88 `*8bbdP*Y8 88       88   *Y888" << std::endl;
    prnt::out << "                         88" << std::endl;
    prnt::out << "                         88                                               *j*m" << std::endl;
    prnt::out << "\n                                                            a uci chess engine" << std::endl;
    prnt::out << "                                                                     v: " << ELEPHANT_GAMBIT_VERSION_STR << std::endl;
    prnt::out << "                                                                      " << ELEPHANT_GAMBIT_GIT_HASH << std::endl;

#ifdef DEVELOPMENT_BUILD
    prnt::out << "---------DEVELOPMENT BUILD---------" << std::endl;
    prnt::out << " versions numbers:" << std::endl;
    prnt::out << " engine: " << ELEPHANT_GAMBIT_VERSION_STR << ELEPHANT_GAMBIT_VERSION_PRERELEASE << ELEPHANT_GAMBIT_VERSION_SUFFIX << std::endl;
    prnt::out << "    cli: " << ELEPHANT_CLI_VERSION_STR << ELEPHANT_CLI_VERSION_PRERELEASE << ELEPHANT_CLI_VERSION_SUFFIX << std::endl;
    prnt::out << "    git: " << ELEPHANT_GAMBIT_GIT_HASH << std::endl;
    prnt::out << " timestamps:" << std::endl;
    prnt::out << "  cmake:  " << ELEPHANT_GAMBIT_BUILD_TIMESTAMP << std::endl;
    prnt::out << "  binary: " << __DATE__ << " at " << __TIME__ << std::endl;
    prnt::out << "----------------------------------" << std::endl;
#endif
}

void
Application::RunUci() {
    AppContext context;
    context.setState(std::make_unique<UciModeProcessor>());
    context.processInput("");
}

void
Application::Run(char* argv[]) {
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
