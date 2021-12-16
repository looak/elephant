set(MAIN_EXE ElephantGambit)

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