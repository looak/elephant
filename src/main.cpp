#include <string>
#include "cli/inc/elephant_cli.hpp"
#include "util/static_initializer.hpp"
#include "cli/inc/static_initializer.hpp"
#include "elephant_gambit_config.h"

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
    try {
        WeightStore::get()->loadFromFile(std::format("{}/res/weights.ini", ROOT_PATH));
    }
    catch (const std::exception& ex) {
        spdlog::error("Failed to load weights file: {}", ex.what());
    }


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
