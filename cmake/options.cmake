# Build options. A build directory keeps what its cache says: -D<OPTION>=<value> and reconfigure to change one. Applied
# in the top-level CMakeLists.txt, ob_build/makefile defines none of them.

# Assertions & exceptions are always on in Debug builds. These add them to the optimized builds as well, on by default:
# a development build catches what the tests and the tuner run into. Turn both off to run (and profile) Release like
# the OpenBench build.
option(ASSERTIONS_ENABLED "Enable assertions in optimized builds, always on in Debug" ON)
option(EXCEPTIONS_ENABLED "Enable exceptions in optimized builds, always on in Debug" ON)

# The log always goes to the console, this also writes it and a copy of std::cout to logs/.
option(OUTPUT_LOG_TO_FILE "Also write the log and std::cout to files in logs/" OFF)

# Target the build machine's instruction set (popcnt, tzcnt, lzcnt...). Turn off for portable binaries, the release
# binaries are built by ob_build/makefile with an explicit ARCH.
option(ELEPHANT_NATIVE_ARCH "Optimize for the build machine's instruction set" ON)

# Instrument engine, cli, phant-indexer & ElephantTest for line, function & branch coverage, gcc's --coverage or
# clang's source based coverage, see cmake/coverage.cmake. Turns link time optimization off. test/coverage.ps1 and
# test/coverage.sh configure a build-cov directory with it, run the unit tests and write the report.
option(ELEPHANT_COVERAGE "Instrument our targets for test coverage, no link time optimization" OFF)

# The log's level, spdlog compiles out everything below it. A level's position in the list is spdlog's
# SPDLOG_LEVEL_* value.
set(LOG_LEVELS trace debug info warn error critical off)
set(LOG_LEVEL "info" CACHE STRING "Log level: trace, debug, info, warn, error, critical or off")
set_property(CACHE LOG_LEVEL PROPERTY STRINGS ${LOG_LEVELS})
list(FIND LOG_LEVELS "${LOG_LEVEL}" SPDLOG_ACTIVE_LEVEL)
if(SPDLOG_ACTIVE_LEVEL EQUAL -1)
    message(FATAL_ERROR "Invalid LOG_LEVEL '${LOG_LEVEL}', one of: ${LOG_LEVELS}")
endif()

# Debugging switches, forced off: a build that needs one turns it on here (or in a CMake file of its own), they can't be
# set from the command line.
set(DEVELOPMENT_BUILD OFF CACHE BOOL "CLI banner with the version & build details" FORCE)
set(DEBUG_TRANSITION_TABLE OFF CACHE BOOL "Transposition table statistics & suspicious score checks" FORCE)
set(DEBUG_SEARCH_TREE OFF CACHE BOOL "Write the search tree to logs/trace/search.log" FORCE)

# Each one that is ON becomes a compile definition of the same name.
set(PRECOMPILE_OPTIONS
    ASSERTIONS_ENABLED
    DEBUG_TRANSITION_TABLE
    DEVELOPMENT_BUILD
    EXCEPTIONS_ENABLED
    OUTPUT_LOG_TO_FILE
    DEBUG_SEARCH_TREE
)
