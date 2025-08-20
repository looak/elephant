set(TEST_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(TEST_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(ElephantGambit_VERSION_SUFFIX "-dbg")
elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    set(ElephantGambit_VERSION_SUFFIX "-dbg")
endif()

configure_file(${TEST_INC_DIR}/elephant_gambit_config.h.in ${TEST_INC_DIR}/elephant_gambit_config.h)

set(TEST_SOURCE_INCLUDE ${TEST_SOURCE_INCLUDE}

)

set(TEST_SOURCE ${TEST_SOURCE}
## /...
    ${TEST_SRC_DIR}/bitboard_test.cpp
    ${TEST_SRC_DIR}/castling_enpassant_test.cpp
    ${TEST_SRC_DIR}/checkmate_test.cpp
    ${TEST_SRC_DIR}/chessboard_test.cpp
    ${TEST_SRC_DIR}/elephant_test.cpp
    ${TEST_SRC_DIR}/elephant_test_utils.h
    ${TEST_SRC_DIR}/fen_parser_test.cpp
    ${TEST_SRC_DIR}/game_context_test.cpp
    ${TEST_SRC_DIR}/material_topology_test.cpp
    ${TEST_SRC_DIR}/move_bulk_generator_test.cpp
    ${TEST_SRC_DIR}/move_executor_test.cpp
    ${TEST_SRC_DIR}/move_gen_isolation_test.cpp
    ${TEST_SRC_DIR}/move_generator_test.cpp
    ${TEST_SRC_DIR}/move_test.cpp
    ${TEST_SRC_DIR}/notation_test.cpp
    ${TEST_SRC_DIR}/perft_test.cpp
    ${TEST_SRC_DIR}/piece_test.cpp
    ${TEST_SRC_DIR}/position_proxy_test.cpp
    ${TEST_SRC_DIR}/position_test.cpp
    ${TEST_SRC_DIR}/rays_test.cpp
    ${TEST_SRC_DIR}/san_parser_test.cpp
    ${TEST_SRC_DIR}/search_test.cpp
    ${TEST_SRC_DIR}/transposition_test.cpp
    ${TEST_SRC_DIR}/uci_test.cpp
    ${TEST_SRC_DIR}/unmake_test.cpp
    ${TEST_SRC_DIR}/weight_store_test.cpp
    ${TEST_SRC_DIR}/zorbist_hashing_test.cpp
)

set(TEST_SOURCE_ALL ${TEST_SOURCE} ${TEST_SOURCE_INCLUDE})
