set(DEVELOPMENT_BUILD ON CACHE STRING "Enable development build settings" FORCE)
set(DEBUG_TRANSITION_TABLE OFF CACHE STRING "Enable debug output for transition table" FORCE)
set(DEBUG_LOGGING_ENABLED OFF CACHE STRING "Enable debug logging" FORCE)
set(LOGGING_ENABLED ON CACHE STRING "Enable logging" FORCE)
set(FATAL_ASSERTS_ENABLED OFF CACHE STRING "Enable fatal assert" FORCE)

set(ENABLE_TRANSPOSITION_TABLE ON CACHE STRING "Enable fatal assert" FORCE)
set(ENABLE_LATE_MOVE_REDUCTION ON CACHE STRING "Enable late move reduction" FORCE)


set(PRECOMPILE_OPTIONS
    DEVELOPMENT_BUILD
    DEBUG_TRANSITION_TABLE
    DEBUG_LOGGING_ENABLED
    LOGGING_ENABLED
    FATAL_ASSERTS_ENABLED
    ENABLE_TRANSPOSITION_TABLE
    ENABLE_LATE_MOVE_REDUCTION
)

# Library to hold our shared compiler warning flags
add_library(common_warnings INTERFACE)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(PROJECT_WARNING_FLAGS
        "-Wall"                # Enable all core warnings
        "-Wextra"              # Enable extra warnings
        "-pedantic"            # Enforce strict standard conformance
        "-Wno-c++98-compat"          # Silence irrelevant C++98 compatibility warnings
        "-Wno-c++98-compat-pedantic" # Silence more C++98 noise, often from macros
    )
    message(STATUS "## Applying GCC/Clang warning flags.")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(PROJECT_WARNING_FLAGS
        "/W4"                  # High warning level, similar to -Wall/-Wextra
        "/permissive-"         # Strict conformance, similar to -pedantic
    )
    message(STATUS "## Applying MSVC warning flags.")
endif()


message(STATUS "## Warning Level: ${WARNING_LEVEL_ARGS}")
target_compile_options(common_warnings INTERFACE ${WARNING_LEVEL_ARGS})