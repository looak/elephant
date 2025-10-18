# Add source to this project's executable.
add_library(${CLI_LIB} "")

target_compile_features(${CLI_LIB} PRIVATE cxx_std_20)
set_target_properties(${CLI_LIB} PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(${CLI_LIB} PROPERTIES CXX_STANDARD_REQUIRED ON)

project(ElephantCli VERSION 0.6.1)
set(ElephantCli_VERSION_PRERELEASE "")

include(sourcelist.cmake)

include_directories(../engine/inc/)
include_directories(src)

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