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

set(TEST_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(TEST_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)


set(TEST_SOURCE ${TEST_SOURCE}
# Main/Entry test file
${TEST_SRC_DIR}/elephant_test.cpp 

# Test files
${TEST_SRC_DIR}/bitboard_test.cpp
${TEST_SRC_DIR}/bulk_move_generator_test.cpp
${TEST_SRC_DIR}/castling_enpassant_test.cpp
#${TEST_SRC_DIR}/checkmate_test.cpp
#${TEST_SRC_DIR}/chessboard_test.cpp
#${TEST_SRC_DIR}/fen_parser_test.cpp
#${TEST_SRC_DIR}/game_context_test.cpp
${TEST_SRC_DIR}/notation_test.cpp
${TEST_SRC_DIR}/material_topology_test.cpp
#${TEST_SRC_DIR}/move_test.cpp
#${TEST_SRC_DIR}/move_generator_test.cpp
#${TEST_SRC_DIR}/perft_test.cpp
${TEST_SRC_DIR}/piece_test.cpp
#${TEST_SRC_DIR}/position_test.cpp
#${TEST_SRC_DIR}/position_proxy_test.cpp
${TEST_SRC_DIR}/rays_test.cpp
#${TEST_SRC_DIR}/search_test.cpp
#${TEST_SRC_DIR}/search_cases.hpp
#${TEST_SRC_DIR}/transposition_test.cpp
#${TEST_SRC_DIR}/unmake_test.cpp
#${TEST_SRC_DIR}/uci_test.cpp
${TEST_SRC_DIR}/weight_store_test.cpp

# Utility files
${TEST_SRC_DIR}/elephant_test_utils.h

${TEST_SRC_DIR}/chess_positions.hpp
)