set(SRC_DIR "src")
set(INC_DIR "inc")

set(CLI_SOURCE ${CLI_SOURCE}
${SRC_DIR}/elephant_cli.cpp 
${SRC_DIR}/commands_print.cpp)

set(CLI_SOURCE_INCLUDE ${CLI_SOURCE_INCLUDE}
${INC_DIR}/elephant_cli.h)

target_sources(${LIBRARY_NAME}
    PRIVATE
        ${CLI_SOURCE}
    PUBLIC
        ${CLI_SOURCE_INCLUDE}
)

target_include_directories(${LIBRARY_NAME}
    PUBLIC
        ${INC_DIR}
)