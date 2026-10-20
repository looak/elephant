set(DEVELOPMENT_BUILD OFF CACHE BOOL "Enable development build settings" FORCE)
# Off by default, logging goes to stdout and a UCI GUI reads stdout. UCI output has its own logger and isn't affected,
# failed asserts print to stderr. A build directory keeps what its cache says, -DLOG_LEVEL=info to turn it back on.
set(LOG_LEVEL "off" CACHE STRING "Set the logging level (trace, debug, info, warn, error, critical, off)")

# Assertions & exceptions are always on in Debug builds. These add them to the optimized builds as well, on by default:
# a development build catches what the tests and the tuner run into. Turn both off to run (and profile) Release like
# the OpenBench build, ob_build/makefile defines neither. A build directory keeps what its cache says.
set(ASSERTIONS_ENABLED ON CACHE BOOL "Enable assertions in optimized builds, always on in Debug")
set(EXCEPTIONS_ENABLED ON CACHE BOOL "Enable exceptions in optimized builds, always on in Debug")

set(OUTPUT_LOG_TO_FILE OFF CACHE BOOL "Output log to file" FORCE)
set(DEBUG_TRANSITION_TABLE OFF CACHE BOOL "Enable debug output for transition table" FORCE)
set(DEBUG_SEARCH_TREE OFF CACHE BOOL "Enable debug file output for search tree" FORCE)

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
    DEBUG_SEARCH_TREE
)

# Library to hold our shared compiler warning flags
add_library(common_warnings INTERFACE)

if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    # cl.exe and clang-cl: MSVC-style driver. Check this first, clang-cl reports COMPILER_ID "Clang".
    set(PROJECT_WARNING_FLAGS
        "/W4"                  # High warning level, similar to -Wall/-Wextra
        "/permissive-"         # Strict conformance, similar to -pedantic
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # clang-cl passes -W flags through to clang, so the extras still apply.
        list(APPEND PROJECT_WARNING_FLAGS
            "-Wno-c++98-compat"          # Silence irrelevant C++98 compatibility warnings
            "-Wno-c++98-compat-pedantic" # Silence more C++98 noise, often from macros
            "-Wshadow"          # Warn on variable shadowing
            "-Wconversion"      # Warn on type conversions that may alter a value
            "-Wsign-conversion" # Warn on sign conversions
        )
    endif()
    message(STATUS "## Applying MSVC-style warning flags.")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # gcc, clang, clang++: GNU-style driver.
    set(PROJECT_WARNING_FLAGS
        "-Wall"                # Enable all core warnings (includes unused variable/parameter)
        "-Wextra"              # Enable extra warnings
        "-pedantic"            # Enforce strict standard conformance
        "-Wno-c++98-compat"          # Silence irrelevant C++98 compatibility warnings
        "-Wno-c++98-compat-pedantic" # Silence more C++98 noise, often from macros
        "-Wshadow"          # Warn on variable shadowing
        "-Wconversion"      # Warn on type conversions that may alter a value
        "-Wsign-conversion" # Warn on sign conversions
    )
    message(STATUS "## Applying GCC/Clang warning flags.")
endif()


message(STATUS "## Warning Level: ${PROJECT_WARNING_FLAGS}")
target_compile_options(common_warnings INTERFACE ${PROJECT_WARNING_FLAGS})
add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})
