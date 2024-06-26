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

set(SRC_DIR "src")
set(INC_DIR "src/inc")

set(TEST_SOURCE ${TEST_SOURCE}
# Main/Entry test file
${SRC_DIR}/elephant_test.cpp 

# Test files
${SRC_DIR}/bitboard_test.cpp
${SRC_DIR}/checkmate_test.cpp
${SRC_DIR}/chessboard_test.cpp
${SRC_DIR}/fen_parser_test.cpp
${SRC_DIR}/game_context_test.cpp
${SRC_DIR}/move_test.cpp
${SRC_DIR}/move_generator_test.cpp
${SRC_DIR}/perft_test.cpp
${SRC_DIR}/piece_test.cpp
${SRC_DIR}/position_test.cpp
${SRC_DIR}/rays_test.cpp
${SRC_DIR}/search_test.cpp
${SRC_DIR}/search_cases.hpp
${SRC_DIR}/transposition_test.cpp
${SRC_DIR}/unmake_test.cpp
${SRC_DIR}/uci_test.cpp

# Utility files
${SRC_DIR}/elephant_test_util_funcs.cpp 
${SRC_DIR}/elephant_test_utils.h)