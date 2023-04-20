# Elephant Gambit Chess Engine - a Chess AI
# Copyright (C) 2021-2023  Alexander Loodin Ek

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

# CMakeList.txt : Top-level CMake project file
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
cmake_policy(SET CMP0135 NEW)

project(ElephantGambit VERSION 0.1.0 LANGUAGES CXX)
set(ElephantGambit_VERSION_PRERELEASE "alpha.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warning levels
if(MSVC)
    # For Microsoft Visual Studio
    set(WARNING_LEVEL "/W4")
    set(WARNING_LEVEL_PEDANTIC "/W4 /permissive-")
else()
    # For GCC and Clang
    set(WARNING_LEVEL "-Wall -Wextra - Wpedantic -Wno-unused-parameter")
endif()

message(STATUS "## Warning level: ${WARNING_LEVEL}")

include(GNUInstallDirs)

foreach(CONFIG_TYPE IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    message(STATUS "## Configuring for ${CONFIG_TYPE} build type")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/${CONFIG_TYPE}/${CMAKE_INSTALL_LIBDIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/${CONFIG_TYPE}/${CMAKE_INSTALL_LIBDIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/${CONFIG_TYPE}/${CMAKE_INSTALL_BINDIR})
endforeach()

# defines targets and sources
add_subdirectory(src)

# enable testing and define tests
enable_testing()
add_subdirectory(test)
