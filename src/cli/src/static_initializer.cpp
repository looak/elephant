#include "static_initializer.hpp"

#include "commands/commands.hpp"

namespace elephant {

bool static_initialize()
{
    register_FenCommand();
    register_NewGameCommand();
	// Perform any necessary static initialization here.
	// This function will be called before main() starts.
	return true;
}

}  // namespace elephant