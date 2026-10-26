# Compiler warnings, linked as the common_warnings interface library by every target of ours.

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
        "-Wshadow"          # Warn on variable shadowing
        "-Wconversion"      # Warn on type conversions that may alter a value
        "-Wsign-conversion" # Warn on sign conversions
    )
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
