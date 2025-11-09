set(VAR_ELEPHANT_MAJOR 0 CACHE INTERNAL "Major Version")
set(VAR_ELEPHANT_MINOR 10 CACHE INTERNAL "Minor Version")
set(VAR_ELEPHANT_PATCH 0 CACHE INTERNAL "Patch Version")
set(VAR_ELEPHANT_VERSION_SUFFIX "" CACHE STRING "Version suffix")
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