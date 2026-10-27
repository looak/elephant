# --- Version from version.txt ---
# version.txt is the only place the version is written, ob_build/makefile reads it too. Line 1 holds
# major.minor.patch with an optional pre-release label naming the feature being worked on, e.g. 0.12.1-eval. Line 2,
# when there is one, is the release's codename, e.g. Matriarch: letters, digits, spaces, dots, underscores & hyphens.
set(VAR_ELEPHANT_VERSION_FILE ${CMAKE_CURRENT_LIST_DIR}/../version.txt)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${VAR_ELEPHANT_VERSION_FILE})
file(STRINGS ${VAR_ELEPHANT_VERSION_FILE} VAR_ELEPHANT_VERSION_FULL LIMIT_COUNT 1)
string(STRIP "${VAR_ELEPHANT_VERSION_FULL}" VAR_ELEPHANT_VERSION_FULL)

file(STRINGS ${VAR_ELEPHANT_VERSION_FILE} VAR_ELEPHANT_VERSION_LINES LIMIT_COUNT 2)
list(LENGTH VAR_ELEPHANT_VERSION_LINES VAR_ELEPHANT_VERSION_LINE_COUNT)
set(VAR_ELEPHANT_CODENAME "")
if(VAR_ELEPHANT_VERSION_LINE_COUNT GREATER 1)
    list(GET VAR_ELEPHANT_VERSION_LINES 1 VAR_ELEPHANT_CODENAME)
    string(STRIP "${VAR_ELEPHANT_CODENAME}" VAR_ELEPHANT_CODENAME)
endif()
if(NOT VAR_ELEPHANT_CODENAME MATCHES "^[A-Za-z0-9 ._-]*$")
    message(FATAL_ERROR "version.txt line 2 is the codename, letters, digits, spaces, . _ and -, got '${VAR_ELEPHANT_CODENAME}'")
endif()

if(NOT VAR_ELEPHANT_VERSION_FULL MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-[0-9A-Za-z.-]+)?$")
    message(FATAL_ERROR "version.txt must hold major.minor.patch[-label], got '${VAR_ELEPHANT_VERSION_FULL}'")
endif()

set(VAR_ELEPHANT_MAJOR ${CMAKE_MATCH_1} CACHE INTERNAL "Major Version")
set(VAR_ELEPHANT_MINOR ${CMAKE_MATCH_2} CACHE INTERNAL "Minor Version")
set(VAR_ELEPHANT_PATCH ${CMAKE_MATCH_3} CACHE INTERNAL "Patch Version")
# includes the leading dash, "-eval", or empty for a release.
set(VAR_ELEPHANT_VERSION_PRERELEASE "${CMAKE_MATCH_4}" CACHE INTERNAL "Pre-release label")
set(VAR_ELEPHANT_VERSION ${VAR_ELEPHANT_MAJOR}.${VAR_ELEPHANT_MINOR}.${VAR_ELEPHANT_PATCH} CACHE INTERNAL "Version")

# The git commit is written on every build by cmake/build_info.cmake, at configure time it goes stale.

# --- Add Build Timestamp ---
string(TIMESTAMP VAR_ELEPHANT_BUILD_TIMESTAMP "%b %d %Y at %H:%M:%S")