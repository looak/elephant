# The UCI protocol: a library the CLI's uci mode and the tests share, and eg-uci, the binary that is only that. Nothing
# in the engine knows about it, nothing here knows about the CLI.
add_library(${UCI_LIB} STATIC
    src/uci.cpp
    src/uci_commands.cpp
    src/uci_loop.cpp
    src/uci_printer.cpp
    src/uci_search_listener.cpp)

target_compile_features(${UCI_LIB} PUBLIC cxx_std_20)
set_target_properties(${UCI_LIB} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

target_include_directories(${UCI_LIB} PUBLIC inc)

target_link_libraries(${UCI_LIB}
    PUBLIC
        ${ENGINE_LIB}
        spdlog
    PRIVATE
        common_warnings
        coverage_flags)

add_executable(eg-uci src/main.cpp)

set_target_properties(eg-uci PROPERTIES
    OUTPUT_NAME "eg-uci-${ELEPHANT_BINARY_SUFFIX}"
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

target_link_libraries(eg-uci PRIVATE common_warnings ${UCI_LIB})
