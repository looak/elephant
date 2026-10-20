# Elephant Gambit Chess Engine - a Chess AI
# Copyright (C) 2021-2026  Alexander Loodin Ek

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

# PhantIndexer builds phant-indexer, which replays PGN games and writes an index entry per game for nyayo's game
# catalogue. PhantIndexerLib is the replay & the entry, linked by the executable and by ElephantTest.
set(INDEXER_LIB PhantIndexerLib)
set(INDEXER_EXE PhantIndexer)

# the engine's parsers report an illegal move by throwing, see game_index.cpp.
if(NOT EXCEPTIONS_ENABLED)
    message(STATUS "## PhantIndexer: OFF, it needs EXCEPTIONS_ENABLED")
    return()
endif()
message(STATUS "## PhantIndexer: ON")

add_library(${INDEXER_LIB} STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/game_index.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_index.cpp)
target_compile_features(${INDEXER_LIB} PUBLIC cxx_std_20)
set_target_properties(${INDEXER_LIB} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
target_include_directories(${INDEXER_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(${INDEXER_LIB} PUBLIC engine spdlog PRIVATE common_warnings)

add_executable(${INDEXER_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
set_target_properties(${INDEXER_EXE} PROPERTIES
    OUTPUT_NAME phant-indexer
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
target_link_libraries(${INDEXER_EXE} PRIVATE ${INDEXER_LIB} common_warnings)
