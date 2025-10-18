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

include(sourcelist.cmake)

add_executable(${PROJECT_NAME} ${TEST_SOURCE_ALL})

# Explicitly set the C++ standard for test target
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES 
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

target_include_directories(${PROJECT_NAME} 
  PRIVATE
    ${TEST_INC_DIR})
  
target_link_libraries(${PROJECT_NAME} common_warnings gtest_main engine)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

target_include_directories(ElephantTest
    PRIVATE
        # This tells the compiler to ignore warnings from Google Test's headers
        SYSTEM "${CMAKE_BINARY_DIR}/_deps/googletest-src/googletest/include"
        SYSTEM "${CMAKE_BINARY_DIR}/_deps/googletest-src/googlemock/include"
)