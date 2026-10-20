#include <format>
#include <string>
#include "cli/inc/elephant_cli.hpp"
#include "system/static_initializer.hpp"
#include "cli/inc/static_initializer.hpp"
#include "elephant_gambit_config.h"
#include <diagnostics/logger.hpp>
#ifdef OUTPUT_LOG_TO_FILE
#include <diagnostics/log_redirect.hpp>
#endif

#include <spdlog/spdlog.h>

bool engine_initialized = static_initializer::initialize();
bool cli_initialized = elephant::static_initialize();

int
main(int argc, char* argv[])
{    
    logging::init();
#ifdef OUTPUT_LOG_TO_FILE
    logging::ScopedDualRedirect redirectCout(std::cout, logging::readCoutFilename());
#endif

    ASSERT(engine_initialized);
    ASSERT(cli_initialized);


    if (argc > 1) {
        std::string argument(argv[1]);
        if (argument == "uci") {
            AppContext context;
            context.setState(std::make_unique<UciModeProcessor>());
            context.processInput("");
        }
        else {
            Application app;
            app.Run(argv);
        }        
    }
    else {
        Application app;
        app.Run(nullptr);
    }

    logging::deinit();
    return 0;
}
