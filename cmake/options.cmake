set(DEVELOPMENT_BUILD OFF CACHE BOOL "Enable development build settings" FORCE)
set(LOG_LEVEL "info" CACHE STRING "Set the logging level (trace, debug, info, warn, error, critical, off)")

set(ASSERTIONS_ENABLED ON CACHE BOOL "Enable assertions" FORCE)
set(EXCEPTIONS_ENABLED ON CACHE BOOL "Enable exceptions" FORCE)

set(OUTPUT_LOG_TO_FILE OFF CACHE BOOL "Output log to file" FORCE)
set(DEBUG_TRANSITION_TABLE ON CACHE BOOL "Enable debug output for transition table" FORCE)

# Map to spdlog's compile-time level
if(${LOG_LEVEL} STREQUAL "trace")
    set(SPDLOG_ACTIVE_LEVEL 0)
elseif(${LOG_LEVEL} STREQUAL "debug")
    set(SPDLOG_ACTIVE_LEVEL 1)
elseif(${LOG_LEVEL} STREQUAL "info")
    set(SPDLOG_ACTIVE_LEVEL 2)
elseif(${LOG_LEVEL} STREQUAL "warn")
    set(SPDLOG_ACTIVE_LEVEL 3)
elseif(${LOG_LEVEL} STREQUAL "error")
    set(SPDLOG_ACTIVE_LEVEL 4)
elseif(${LOG_LEVEL} STREQUAL "critical")
    set(SPDLOG_ACTIVE_LEVEL 5)
elseif(${LOG_LEVEL} STREQUAL "off")
    set(SPDLOG_ACTIVE_LEVEL 6)
else()
    message(FATAL_ERROR "Invalid LOG_LEVEL: ${LOG_LEVEL}")
endif()

set(PRECOMPILE_OPTIONS
    ASSERTIONS_ENABLED
    DEBUG_TRANSITION_TABLE
    DEVELOPMENT_BUILD
    EXCEPTIONS_ENABLED
    OUTPUT_LOG_TO_FILE
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
add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})
