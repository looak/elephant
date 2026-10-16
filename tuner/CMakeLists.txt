# Elephant Gambit Chess Engine - a Chess AI
# Copyright (C) 2025  Alexander Loodin Ek

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

# ElephantTuner, the offline evaluation tuner. Reads positions labelled with game results and fits the evaluation's
# TaperedScore values to them, linked against the engine library so it evaluates exactly like the engine does.
# Not part of the OpenBench makefile build.

set(TUNER_EXE ElephantTuner)
set(TUNER_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(TUNER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(sourcelist.cmake)

add_executable(${TUNER_EXE} ${TUNER_SOURCE_ALL})

target_compile_features(${TUNER_EXE} PRIVATE cxx_std_20)
set_target_properties(${TUNER_EXE} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

target_include_directories(${TUNER_EXE} PRIVATE ${TUNER_INC_DIR})

target_link_libraries(${TUNER_EXE} PRIVATE common_warnings engine spdlog)
