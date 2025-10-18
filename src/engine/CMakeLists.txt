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
add_library(${ENGINE_LIB} STATIC)

target_compile_features(${ENGINE_LIB} PRIVATE cxx_std_20)
set_target_properties(${ENGINE_LIB} PROPERTIES 
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

include(sourcelist.cmake)

target_sources(${ENGINE_LIB}
    PRIVATE
        ${ENGINE_SOURCE}
    PUBLIC
        ${ENGINE_SOURCE_INCLUDE}
)

target_include_directories(${ENGINE_LIB}
    PUBLIC
        ${ENGINE_INC_DIR}
)

target_link_libraries(${ENGINE_LIB}
    PRIVATE
        common_warnings
)