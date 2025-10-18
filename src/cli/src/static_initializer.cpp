#include "static_initializer.hpp"

#include "commands/commands.hpp"

namespace elephant {

// Performs necessary static initialization.
// This function will be called before main() starts.
// at some point this will be generated
bool static_initialize()
{
    register_FenCommand();
    register_NewGameCommand();
    register_AboutCommand();
    register_ExitCommand();
    register_HelpCommand();
	
	return true;
}

}  // namespace elephant