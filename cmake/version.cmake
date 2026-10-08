# --- Version from version.txt ---
# version.txt is the only place the version is written, ob_build/makefile reads it too. It holds
# major.minor.patch with an optional pre-release label naming the feature being worked on, e.g. 0.12.1-eval.
set(VAR_ELEPHANT_VERSION_FILE ${CMAKE_CURRENT_LIST_DIR}/../version.txt)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${VAR_ELEPHANT_VERSION_FILE})
file(STRINGS ${VAR_ELEPHANT_VERSION_FILE} VAR_ELEPHANT_VERSION_FULL LIMIT_COUNT 1)
string(STRIP "${VAR_ELEPHANT_VERSION_FULL}" VAR_ELEPHANT_VERSION_FULL)

if(NOT VAR_ELEPHANT_VERSION_FULL MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-[0-9A-Za-z.-]+)?$")
    message(FATAL_ERROR "version.txt must hold major.minor.patch[-label], got '${VAR_ELEPHANT_VERSION_FULL}'")
endif()

set(VAR_ELEPHANT_MAJOR ${CMAKE_MATCH_1} CACHE INTERNAL "Major Version")
set(VAR_ELEPHANT_MINOR ${CMAKE_MATCH_2} CACHE INTERNAL "Minor Version")
set(VAR_ELEPHANT_PATCH ${CMAKE_MATCH_3} CACHE INTERNAL "Patch Version")
# includes the leading dash, "-eval", or empty for a release.
set(VAR_ELEPHANT_VERSION_PRERELEASE "${CMAKE_MATCH_4}" CACHE INTERNAL "Pre-release label")
set(VAR_ELEPHANT_VERSION ${VAR_ELEPHANT_MAJOR}.${VAR_ELEPHANT_MINOR}.${VAR_ELEPHANT_PATCH} CACHE INTERNAL "Version")

# --- Add Git Hash ---
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VAR_ELEPHANT_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if(NOT VAR_ELEPHANT_GIT_HASH)
    set(VAR_ELEPHANT_GIT_HASH "nogit") # Fallback if not a git repo or git fails
endif()

# --- Add Build Timestamp ---
string(TIMESTAMP VAR_ELEPHANT_BUILD_TIMESTAMP "%b %d %Y at %H:%M:%S")