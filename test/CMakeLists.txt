# Elephant Gambit Chess Engine - a Chess AI
# Copyright (C) 2021  Alexander Loodin Ek

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

message(STATUS "## Enabling testing")
set(PROJECT_NAME ElephantTest)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Silence compiler warnings in third-party googletest sources
foreach(gtest_target gtest gtest_main gmock gmock_main)
    if(TARGET ${gtest_target})
        target_compile_options(${gtest_target} PRIVATE $<IF:$<BOOL:${MSVC}>,/w,-w>)
    endif()
endforeach()

include(sourcelist.cmake)

option(ELEPHANT_BUILD_TEST_SUITE "Build ElephantSuite, the long running perft & EPD correctness tests" ON)

include(GoogleTest)

# Defines a gtest executable and registers its tests with ctest under the given label,
# so each set can be run on its own with: ctest -L <label>
# usage: elephant_add_test_executable(<target> LABEL <label> [TIMEOUT <seconds>] SOURCES <files...>)
function(elephant_add_test_executable target)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "LABEL;TIMEOUT" "SOURCES")
    if(NOT ARG_TIMEOUT)
        set(ARG_TIMEOUT 1500) # ctest's default
    endif()

    add_executable(${target} ${ARG_SOURCES})

    # Explicitly set the C++ standard for test target
    target_compile_features(${target} PRIVATE cxx_std_20)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

    target_include_directories(${target}
      PRIVATE
        ${TEST_INC_DIR}
        # This tells the compiler to ignore warnings from Google Test's headers
        SYSTEM "${CMAKE_BINARY_DIR}/_deps/googletest-src/googletest/include"
        SYSTEM "${CMAKE_BINARY_DIR}/_deps/googletest-src/googlemock/include")

    target_link_libraries(${target} common_warnings gtest_main engine spdlog)

    gtest_discover_tests(${target} PROPERTIES LABELS ${ARG_LABEL} TIMEOUT ${ARG_TIMEOUT})
endfunction()

# Fast unit tests, run on every push.
elephant_add_test_executable(${PROJECT_NAME} LABEL unit SOURCES ${TEST_SOURCE_ALL})

# Perft & EPD suites, each test can take minutes.
if(ELEPHANT_BUILD_TEST_SUITE)
    elephant_add_test_executable(ElephantSuite LABEL suite TIMEOUT 3600 SOURCES ${TEST_SUITE_SOURCE_ALL})
endif()
