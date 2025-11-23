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

set(MAIN_EXE ElephantGambit)

add_executable (${MAIN_EXE} main.cpp)

target_compile_features(${MAIN_EXE} PUBLIC cxx_std_20)
set_target_properties(${MAIN_EXE} PROPERTIES 
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

set(ENGINE_LIB engine)
set(CLI_LIB cli)

# Include sub-projects.
add_subdirectory("cli")
add_subdirectory("engine")

target_link_libraries(${MAIN_EXE}
  PUBLIC
  libpopcnt  
  PRIVATE
  spdlog
  ${CLI_LIB}
  ${ENGINE_LIB}
)