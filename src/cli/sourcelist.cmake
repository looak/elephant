set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(ElephantCli_VERSION_SUFFIX "-dbg")
elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    set(ElephantCli_VERSION_SUFFIX "-dbg")
endif()

configure_file(${INC_DIR}/elephant_cli_config.h.in ${INC_DIR}/elephant_cli_config.h)

set(CLI_SOURCE_INCLUDE ${CLI_SOURCE_INCLUDE}
${INC_DIR}/elephant_cli.hpp
${INC_DIR}/elephant_cli_config.h

${INC_DIR}/commands/logic/command_processor.hpp
${INC_DIR}/commands/logic/command_registry.hpp

${INC_DIR}/commands/commands.hpp
${INC_DIR}/commands/command_api.hpp
${INC_DIR}/commands/fen_command.hpp
${INC_DIR}/commands/help_command.hpp
${INC_DIR}/commands/new_game_command.hpp
${INC_DIR}/commands/simple_commands.hpp
${INC_DIR}/commands/print_command.hpp
${INC_DIR}/commands/move_command.hpp

${INC_DIR}/printer/printer.hpp

${INC_DIR}/static_initializer.hpp
)

set(CLI_SOURCE ${CLI_SOURCE}
${SRC_DIR}/elephant_cli.cpp
${SRC_DIR}/commands/command_processor.cpp



${SRC_DIR}/static_initializer.cpp

)

#${SRC_DIR}/commands_print.cpp 
#${SRC_DIR}/commands_print.h
#${SRC_DIR}/commands.cpp 
#${SRC_DIR}/commands.h
#${SRC_DIR}/commands_utils.h
#${SRC_DIR}/commands_uci.cpp
#${SRC_DIR}/commands_uci.h )


