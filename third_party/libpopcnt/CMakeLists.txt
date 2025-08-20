# Define a header-only INTERFACE library.
# This doesn't produce a binary, but it can be linked against
# to provide its usage requirements (like include directories).
add_library(libpopcnt INTERFACE)

# Specify that any target linking against libpopcnt should
# have the "include" directory added to its include path.
target_include_directories(libpopcnt
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)