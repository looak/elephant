set(ENGINE_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(ENGINE_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(ElephantGambit_VERSION_SUFFIX "-dbg")
elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    set(ElephantGambit_VERSION_SUFFIX "-dbg")
endif()

configure_file(${ENGINE_INC_DIR}/elephant_gambit_config.h.in ${ENGINE_INC_DIR}/elephant_gambit_config.h)

set(ENGINE_SOURCE_INCLUDE ${ENGINE_SOURCE_INCLUDE}
## /...
    ${ENGINE_INC_DIR}/defines.hpp
    ${ENGINE_INC_DIR}/elephant_gambit.hpp
    ${ENGINE_INC_DIR}/elephant_gambit_config.h

## /bitboard/...
    ${ENGINE_INC_DIR}/bitboard/bitboard.hpp
    ${ENGINE_INC_DIR}/bitboard/bitboard_constants.hpp
    ${ENGINE_INC_DIR}/bitboard/intrinsics.hpp

## /bitboard/attacks/...
    ${ENGINE_INC_DIR}/bitboard/attacks/attacks.hpp
    ${ENGINE_INC_DIR}/bitboard/attacks/magic_constants.hpp

## /bitboard/rays/...
    ${ENGINE_INC_DIR}/bitboard/rays/rays.hpp

## /core/...
    ${ENGINE_INC_DIR}/core/chessboard.hpp
    ${ENGINE_INC_DIR}/core/game_context.hpp
    ${ENGINE_INC_DIR}/core/square_notation.hpp
    ${ENGINE_INC_DIR}/core/uci.hpp

## /debug/...
    ${ENGINE_INC_DIR}/debug/elephant_exceptions.hpp
    ${ENGINE_INC_DIR}/debug/log.hpp

## /eval/...
    ${ENGINE_INC_DIR}/eval/evaluation_table.hpp
    ${ENGINE_INC_DIR}/eval/evaluator.hpp
    ${ENGINE_INC_DIR}/eval/evaluator_data.hpp

## /io/...
    ${ENGINE_INC_DIR}/io/fen_parser.hpp
    ${ENGINE_INC_DIR}/io/pgn_parser.hpp
    ${ENGINE_INC_DIR}/io/printer.hpp
    ${ENGINE_INC_DIR}/io/san_parser.hpp
    ${ENGINE_INC_DIR}/io/weight_store.hpp
    ${ENGINE_INC_DIR}/io/weights.hpp

## /material/...
    ${ENGINE_INC_DIR}/material/chess_piece.hpp
    ${ENGINE_INC_DIR}/material/chess_piece_defines.hpp
    ${ENGINE_INC_DIR}/material/material_mask.hpp
    ${ENGINE_INC_DIR}/material/material_topology.hpp

## /move/...
    ${ENGINE_INC_DIR}/move/move.hpp
    ${ENGINE_INC_DIR}/move/move_executor.hpp

## /move/generation/...
    ${ENGINE_INC_DIR}/move/generation/king_pin_threats.hpp
    ${ENGINE_INC_DIR}/move/generation/move_bulk_generator.hpp
    ${ENGINE_INC_DIR}/move/generation/move_gen_isolation.hpp
    ${ENGINE_INC_DIR}/move/generation/move_generator.hpp
    ${ENGINE_INC_DIR}/move/generation/move_ordering_view.hpp

## /position/...
    ${ENGINE_INC_DIR}/position/castling_state_info.hpp
    ${ENGINE_INC_DIR}/position/en_passant_state_info.hpp
    ${ENGINE_INC_DIR}/position/hash_zobrist.hpp
    ${ENGINE_INC_DIR}/position/position.hpp
    ${ENGINE_INC_DIR}/position/position_access_policies.hpp
    ${ENGINE_INC_DIR}/position/position_accessors.hpp
    ${ENGINE_INC_DIR}/position/position_proxy.hpp

## /search/...
    ${ENGINE_INC_DIR}/search/perft_search.hpp
    ${ENGINE_INC_DIR}/search/search.hpp
    ${ENGINE_INC_DIR}/search/search_constants.hpp
    ${ENGINE_INC_DIR}/search/search_policies.hpp
    ${ENGINE_INC_DIR}/search/search_results.hpp
    ${ENGINE_INC_DIR}/search/transposition_table.hpp
    ${ENGINE_INC_DIR}/search/tt_policies.hpp

## /util/...
    ${ENGINE_INC_DIR}/util/clock.hpp
    ${ENGINE_INC_DIR}/util/static_initializer.hpp
)

set(ENGINE_SOURCE ${ENGINE_SOURCE}
## /...
    ${ENGINE_SRC_DIR}/elephant_gambit.cpp

## /bitboard/...
    ${ENGINE_SRC_DIR}/bitboard/attacks.cpp
    ${ENGINE_SRC_DIR}/bitboard/rays.cpp

## /core/...
    ${ENGINE_SRC_DIR}/core/chessboard.cpp
    ${ENGINE_SRC_DIR}/core/game_context.cpp
    ${ENGINE_SRC_DIR}/core/uci.cpp

## /debug/...
    ${ENGINE_SRC_DIR}/debug/log.cpp

## /eval/...
    ${ENGINE_SRC_DIR}/eval/evaluator.cpp

## /io/...
    ${ENGINE_SRC_DIR}/io/fen_parser.cpp
    ${ENGINE_SRC_DIR}/io/pgn_parser.cpp
    ${ENGINE_SRC_DIR}/io/printer.cpp
    ${ENGINE_SRC_DIR}/io/san_parser.cpp
    ${ENGINE_SRC_DIR}/io/weight_store.cpp

## /material/...
    ${ENGINE_SRC_DIR}/material/chess_piece.cpp
    ${ENGINE_SRC_DIR}/material/material_mask.cpp
    ${ENGINE_SRC_DIR}/material/material_topology.cpp

## /move/...
    ${ENGINE_SRC_DIR}/move/move.cpp
    ${ENGINE_SRC_DIR}/move/move_executor.cpp

## /move/generation/...
    ${ENGINE_SRC_DIR}/move/generation/king_pin_threats.cpp
    ${ENGINE_SRC_DIR}/move/generation/move_gen_isolation.cpp
    ${ENGINE_SRC_DIR}/move/generation/move_generator.cpp

## /position/...
    ${ENGINE_SRC_DIR}/position/hash_zobrist.cpp
    ${ENGINE_SRC_DIR}/position/position.cpp
    ${ENGINE_SRC_DIR}/position/position_proxy.cpp

## /search/...
    ${ENGINE_SRC_DIR}/search/perft_search.cpp
    ${ENGINE_SRC_DIR}/search/search.cpp

## /util/...
    ${ENGINE_SRC_DIR}/util/clock.cpp
)

set(ENGINE_SOURCE_ALL ${ENGINE_SOURCE} ${ENGINE_SOURCE_INCLUDE})
