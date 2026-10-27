# Compiler warnings, linked as the common_warnings interface library by every target of ours.

add_library(common_warnings INTERFACE)

# clang & g++, either driver: classes of bugs rather than style, none of them fires in our code.
set(BUG_WARNING_FLAGS
    "-Wnon-virtual-dtor"     # A class with virtual functions and a non-virtual destructor
    "-Woverloaded-virtual"   # A function that hides a virtual of its base instead of overriding it
    "-Wcast-qual"            # A cast that drops const or volatile
    "-Wformat=2"             # printf-style format strings, including ones that aren't literals
    "-Wimplicit-fallthrough" # A switch case that falls into the next without [[fallthrough]]
    "-Wundef"                # An undefined macro used in #if, it silently reads as 0
)

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
            ${BUG_WARNING_FLAGS}
        )
    endif()
    message(STATUS "## Applying MSVC-style warning flags.")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # gcc, clang, clang++: GNU-style driver.
    set(PROJECT_WARNING_FLAGS
        "-Wall"                # Enable all core warnings (includes unused variable/parameter)
        "-Wextra"              # Enable extra warnings
        "-pedantic"            # Enforce strict standard conformance
        "-Wshadow"          # Warn on variable shadowing
        "-Wconversion"      # Warn on type conversions that may alter a value
        "-Wsign-conversion" # Warn on sign conversions
        ${BUG_WARNING_FLAGS}
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # g++ only, clang has none of them.
        list(APPEND PROJECT_WARNING_FLAGS
            "-Wduplicated-cond"     # The same condition twice in an if / else if chain
            "-Wduplicated-branches" # An if and its else doing the same thing
            "-Wlogical-op"          # Suspicious && and ||, e.g. the same operand on both sides
            "-Wuseless-cast"        # A cast to the type the value already has
        )
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # clang only, g++ has no -Wc++98-compat and reports the unknown -Wno- flags next to any other warning.
        list(APPEND PROJECT_WARNING_FLAGS
            "-Wno-c++98-compat"          # Silence irrelevant C++98 compatibility warnings
            "-Wno-c++98-compat-pedantic" # Silence more C++98 noise, often from macros
        )
    endif()
    message(STATUS "## Applying GCC/Clang warning flags.")
endif()


message(STATUS "## Warning Level: ${PROJECT_WARNING_FLAGS}")
target_compile_options(common_warnings INTERFACE ${PROJECT_WARNING_FLAGS})
