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
cmake_minimum_required(VERSION 3.14...4.0 FATAL_ERROR)

include(cmake/version.cmake)
project(ElephantGambit VERSION ${VAR_ELEPHANT_VERSION} LANGUAGES CXX)
set(ElephantGambit_VERSION_PRERELEASE "${VAR_ELEPHANT_VERSION_PRERELEASE}")
set(ElephantGambit_VERSION_SUFFIX "")
message(STATUS "## Elephant Gambit version: ${VAR_ELEPHANT_VERSION_FULL}")

include(GNUInstallDirs)
include(cmake/options.cmake)

foreach(option IN LISTS PRECOMPILE_OPTIONS)
    if(${${option}})
        message(STATUS "## Precompile option: ${option} ON")
        add_definitions(-D${option})
    else()
        message(STATUS "## Precompile option: ${option} OFF")
    endif()
endforeach()

message(STATUS "## Precompile log level: ${LOG_LEVEL}")

# Link time optimization for optimized builds. Attack table & ray lookups live in .cpp files and are only inlined
# across translation units with LTO, without it every slider lookup is a function call.
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT LANGUAGES CXX)
if(IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    message(STATUS "## Link time optimization: ON (Release, RelWithDebInfo)")
else()
    message(STATUS "## Link time optimization: not supported, ${IPO_OUTPUT}")
endif()

# Target the build machine's instruction set (popcnt, tzcnt, lzcnt...). Turn off for portable binaries, the release
# binaries are built by ob_build/makefile with an explicit ARCH.
option(ELEPHANT_NATIVE_ARCH "Optimize for the build machine's instruction set" ON)
if(ELEPHANT_NATIVE_ARCH)
    if(MSVC)
        add_compile_options(/arch:AVX2)
    else()
        add_compile_options(-march=native)
    endif()
    message(STATUS "## Native instruction set: ON")
endif()

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

foreach(CONFIG_TYPE IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    message(STATUS "## Configuring for ${CONFIG_TYPE} build type")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/${CONFIG_TYPE}/${CMAKE_INSTALL_LIBDIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/${CONFIG_TYPE}/${CMAKE_INSTALL_LIBDIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/${CONFIG_TYPE}/${CMAKE_INSTALL_BINDIR})
endforeach()

# third_party libraries
add_subdirectory(third_party)

# defines targets and sources
add_subdirectory(src)

# enable testing and define tests
enable_testing()
add_subdirectory(test)
