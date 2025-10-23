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
add_library(${CLI_LIB} "")

target_compile_features(${CLI_LIB} PRIVATE cxx_std_20)
set_target_properties(${CLI_LIB} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

project(ElephantCli VERSION 0.6.1)
set(ElephantCli_VERSION_PRERELEASE "")

include(sourcelist.cmake)

include_directories(../engine/inc/)
include_directories(inc)

target_sources(${CLI_LIB}
    PRIVATE
        ${CLI_SOURCE}
    PUBLIC
        ${CLI_SOURCE_INCLUDE}
)

target_include_directories(${CLI_LIB}
    PUBLIC
        ${INC_DIR}
)

target_link_libraries(${CLI_LIB}
    PRIVATE
        common_warnings
)