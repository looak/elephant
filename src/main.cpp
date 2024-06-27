#include <string>
#include "cli/inc/elephant_cli.h"
#include "static_initializer.hpp"
#include "elephant_gambit_config.h"

bool g_initialized = static_initializer::initialize();

int
main(int argc, char* argv[])
{
    assert(g_initialized);
    WeightStore::get()->loadFromFile(std::format("{}/res/weights.ini", ROOT_PATH));

    Application app;

    if (argc > 1) {
        std::string argument(argv[1]);
        if (argument == "uci")
            app.RunUci();
    }

    app.Run();

    return 0;
}
