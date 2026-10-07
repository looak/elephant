#include "static_initializer.hpp"

#include "commands/commands.hpp"

namespace elephant {

// Performs necessary static initialization.
// This function will be called before main() starts.
// at some point this will be generated
bool static_initialize()
{
    register_FenCommand();
    register_BenchCommand();
    register_NewGameCommand();
    register_AboutCommand();
    register_ExitCommand();
    register_HelpCommand();
    register_PrintCommand();
    register_DivideCommand();
    register_MoveCommand();
    register_PerftCommand();
    register_SearchCommand();
    register_EvaluateCommand();
	
	return true;
}

}  // namespace elephant