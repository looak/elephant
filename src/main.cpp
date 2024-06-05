#include <string>
#include "cli/inc/elephant_cli.h"
#include "static_initializer.hpp"

bool g_initialized = static_initializer::initialize();

int
main(int argc, char* argv[])
{
    assert(g_initialized);
    Application app;

    if (argc > 1) {
        std::string argument(argv[1]);
        if (argument == "uci")
            app.RunUci();
    }

    app.Run();

    return 0;
}
