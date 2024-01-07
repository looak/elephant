#include <string>
#include "cli/inc/elephant_cli.h"

int
main(int argc, char* argv[])
{
    Application app;

    if (argc > 1) {
        std::string argument(argv[1]);
        if (argument == "uci")
            app.RunUci();
    }

    app.Run();

    return 0;
}
