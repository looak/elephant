# Add source to this project's executable.
add_library(${CLI_LIB} "")

project(ElephantCli VERSION 0.5.0)
set(ElephantCli_VERSION_PRERELEASE "")

include(sourcelist.cmake)

include_directories(../engine/inc/)

target_sources(${CLI_LIB}
    PRIVATE
        ${CLI_SOURCE}
    PUBLIC
        ${CLI_SOURCE_INCLUDE}
)

target_include_directories(${CLI_LIB}
    PUBLIC
        ${INC_DIR}
)