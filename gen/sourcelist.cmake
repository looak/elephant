set(TEST_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(TEST_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(ElephantGambit_VERSION_SUFFIX "-dbg")
elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    set(ElephantGambit_VERSION_SUFFIX "-dbg")
endif()

configure_file(${TEST_INC_DIR}/elephant_gambit_config.h.in ${TEST_INC_DIR}/elephant_gambit_config.h)

set(TEST_SOURCE_INCLUDE ${TEST_SOURCE_INCLUDE}
## /...
    ${TEST_INC_DIR}/bitboard_test_helpers.hpp
    ${TEST_INC_DIR}/chess_positions.hpp
    ${TEST_INC_DIR}/elephant_test_utils.h
    ${TEST_INC_DIR}/search_cases.hpp
)

set(TEST_SOURCE ${TEST_SOURCE}
## /...
    ${TEST_SRC_DIR}/elephant_test.cpp

## /bitboard/...
    ${TEST_SRC_DIR}/bitboard/bitboard_test.cpp
    ${TEST_SRC_DIR}/bitboard/rays_test.cpp

## /io/...
    ${TEST_SRC_DIR}/io/fen_parser_test.cpp
    ${TEST_SRC_DIR}/io/san_parser_test.cpp
    ${TEST_SRC_DIR}/io/uci_test.cpp
    ${TEST_SRC_DIR}/io/weight_store_test.cpp
    ${TEST_SRC_DIR}/io/zorbist_hashing_test.cpp

## /move/...
    ${TEST_SRC_DIR}/move/checkmate_test.cpp
    ${TEST_SRC_DIR}/move/move_bulk_generator_test.cpp
    ${TEST_SRC_DIR}/move/move_executor_test.cpp
    ${TEST_SRC_DIR}/move/move_gen_isolation_test.cpp
    ${TEST_SRC_DIR}/move/move_generator_test.cpp
    ${TEST_SRC_DIR}/move/move_test.cpp
    ${TEST_SRC_DIR}/move/unmake_test.cpp

## /position/...
    ${TEST_SRC_DIR}/position/castling_enpassant_test.cpp
    ${TEST_SRC_DIR}/position/chessboard_test.cpp
    ${TEST_SRC_DIR}/position/material_topology_test.cpp
    ${TEST_SRC_DIR}/position/notation_test.cpp
    ${TEST_SRC_DIR}/position/piece_test.cpp
    ${TEST_SRC_DIR}/position/position_proxy_test.cpp
    ${TEST_SRC_DIR}/position/position_test.cpp

## /search/...
    ${TEST_SRC_DIR}/search/perft_test.cpp
    ${TEST_SRC_DIR}/search/search_test.cpp
    ${TEST_SRC_DIR}/search/transposition_test.cpp
)

set(TEST_SOURCE_ALL ${TEST_SOURCE} ${TEST_SOURCE_INCLUDE})
