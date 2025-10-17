#include <string>
#include "cli/inc/elephant_cli.h"
#include "util/static_initializer.hpp"
#include "cli/src/static_initializer.hpp"
#include "elephant_gambit_config.h"

bool engine_initialized = static_initializer::initialize();
bool cli_initialized = elephant::static_initialize();

int
main(int argc, char* argv[])
{
    assert(engine_initialized);
    assert(cli_initialized);
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
