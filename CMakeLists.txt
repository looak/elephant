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
set(ElephantGambit_CODENAME "${VAR_ELEPHANT_CODENAME}")
message(STATUS "## Elephant Gambit version: ${VAR_ELEPHANT_VERSION_FULL} \"${VAR_ELEPHANT_CODENAME}\"")

include(GNUInstallDirs)
include(cmake/options.cmake)

foreach(option IN LISTS PRECOMPILE_OPTIONS)
    if(${option})
        message(STATUS "## Precompile option: ${option} ON")
        add_compile_definitions(${option})
    else()
        message(STATUS "## Precompile option: ${option} OFF")
    endif()
endforeach()

# every Debug build checks assertions & throws, whatever the options above say.
add_compile_definitions($<$<CONFIG:Debug>:ASSERTIONS_ENABLED> $<$<CONFIG:Debug>:EXCEPTIONS_ENABLED>)
message(STATUS "## Precompile option: ASSERTIONS_ENABLED & EXCEPTIONS_ENABLED always ON in Debug")

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})
message(STATUS "## Precompile log level: ${LOG_LEVEL}")

include(cmake/warnings.cmake)
include(cmake/coverage.cmake)

# Link time optimization for optimized builds. Attack table & ray lookups live in .cpp files and are only inlined
# across translation units with LTO, without it every slider lookup is a function call. Off for coverage, inlining
# across files blurs which line ran.
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT LANGUAGES CXX)
if(ELEPHANT_COVERAGE)
    message(STATUS "## Link time optimization: OFF (ELEPHANT_COVERAGE)")
elseif(IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    message(STATUS "## Link time optimization: ON (Release, RelWithDebInfo)")
else()
    message(STATUS "## Link time optimization: not supported, ${IPO_OUTPUT}")
endif()

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(cmake/arch.cmake)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# What you run goes in dist/, the objects stay in the build tree. A coverage build keeps its binaries in its tree too, they
# aren't for running and their reports are read from there.
set(ELEPHANT_DIST_DIR "${CMAKE_SOURCE_DIR}/dist")
foreach(CONFIG_TYPE IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    message(STATUS "## Configuring for ${CONFIG_TYPE} build type")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/${CONFIG_TYPE}/${CMAKE_INSTALL_LIBDIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/${CONFIG_TYPE}/${CMAKE_INSTALL_LIBDIR})
    if(ELEPHANT_COVERAGE)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/${CONFIG_TYPE}/${CMAKE_INSTALL_BINDIR})
    elseif(CONFIG_TYPE STREQUAL Debug)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${ELEPHANT_DIST_DIR}/debug)
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${ELEPHANT_DIST_DIR})
    endif()
endforeach()

include(cmake/deps.cmake)

# third_party libraries
add_subdirectory(third_party)

# defines targets and sources
add_subdirectory(src)

option(ELEPHANT_BUILD_TUNER "Build ElephantTuner, the offline evaluation tuner" ON)
if(ELEPHANT_BUILD_TUNER)
    add_subdirectory(src/tuner)
endif()

# phant-indexer, the game reader for nyayo's catalogue. Off by default: it fetches zlib, bzip2 & zstd at configure
# time, which an engine build has no use for.
option(ELEPHANT_BUILD_INDEXER "Build phant-indexer, the game reader for nyayo's catalogue" OFF)
if(ELEPHANT_BUILD_INDEXER)
    add_subdirectory(src/indexer)
endif()

# the tests fetch googletest at configure time, which building the engine has no use for.
option(ELEPHANT_BUILD_TESTS "Build the tests, fetches googletest" ON)
enable_testing()
if(ELEPHANT_BUILD_TESTS)
    add_subdirectory(test)
endif()
