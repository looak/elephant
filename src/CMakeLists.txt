set(MAIN_EXE ElephantGambit)

if(MSVC)
    # For Microsoft Visual Studio
    set(WARNING_LEVEL "/W4")
else()
    # For GCC and Clang
    message(STATUS "## Added -Werror")    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

add_executable (${MAIN_EXE} main.cpp)

set(ENGINE_LIB engine)
set(CLI_LIB cli)

# Include sub-projects.
add_subdirectory("cli")
add_subdirectory("engine")

target_link_libraries(${MAIN_EXE}
  PRIVATE
  ${CLI_LIB}
  ${ENGINE_LIB}
)