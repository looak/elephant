set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)

set(CLI_SOURCE ${CLI_SOURCE}
${SRC_DIR}/elephant_cli.cpp 
${SRC_DIR}/commands_print.cpp 
${SRC_DIR}/commands_print.h
${SRC_DIR}/commands.cpp 
${SRC_DIR}/commands.h
${SRC_DIR}/commands_utils.h)

set(CLI_SOURCE_INCLUDE ${CLI_SOURCE_INCLUDE}
${INC_DIR}/elephant_cli.h)