set(ENGINE_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(ENGINE_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(ElephantGambit_VERSION_SUFFIX "-dbg")
elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    set(ElephantGambit_VERSION_SUFFIX "-dbg")
endif()

configure_file(${ENGINE_INC_DIR}/elephant_gambit_config.h.in ${ENGINE_INC_DIR}/elephant_gambit_config.h)

set(ENGINE_SOURCE_INCLUDE ${ENGINE_SOURCE_INCLUDE}
${ENGINE_INC_DIR}/elephant_gambit.h
${ENGINE_INC_DIR}/elephant_gambit_config.h
${ENGINE_INC_DIR}/defines.h
${ENGINE_INC_DIR}/libpopcnt.h

${ENGINE_INC_DIR}/bitboard.hpp
${ENGINE_INC_DIR}/chessboard.h
${ENGINE_INC_DIR}/chess_piece.h
${ENGINE_INC_DIR}/clock.hpp
${ENGINE_INC_DIR}/evaluation_table.hpp
${ENGINE_INC_DIR}/evaluator.h
${ENGINE_INC_DIR}/fen_parser.h
${ENGINE_INC_DIR}/game_context.h
${ENGINE_INC_DIR}/hash_zorbist.h
${ENGINE_INC_DIR}/intrinsics.hpp
${ENGINE_INC_DIR}/king_pin_threats.hpp
${ENGINE_INC_DIR}/log.h
${ENGINE_INC_DIR}/move.h
${ENGINE_INC_DIR}/notation.h
${ENGINE_INC_DIR}/move_generator.hpp
${ENGINE_INC_DIR}/position.hpp
${ENGINE_INC_DIR}/search.h
${ENGINE_INC_DIR}/transposition_table.hpp
${ENGINE_INC_DIR}/uci.hpp
)

set(ENGINE_SOURCE ${ENGINE_SOURCE}
${ENGINE_SRC_DIR}/elephant_gambit.cpp
${ENGINE_SRC_DIR}/internal_defines.h

${ENGINE_SRC_DIR}/bitboard.cpp
${ENGINE_SRC_DIR}/chessboard.cpp
${ENGINE_SRC_DIR}/chess_piece.cpp
${ENGINE_SRC_DIR}/clock.cpp
${ENGINE_SRC_DIR}/evaluator.cpp
${ENGINE_SRC_DIR}/evaluator_data.h
${ENGINE_SRC_DIR}/fen_parser.cpp
${ENGINE_SRC_DIR}/game_context.cpp
${ENGINE_SRC_DIR}/hash_zorbist.cpp
${ENGINE_SRC_DIR}/king_pin_threats.cpp
${ENGINE_SRC_DIR}/log.cpp
${ENGINE_SRC_DIR}/move.cpp
${ENGINE_SRC_DIR}/notation.cpp
${ENGINE_SRC_DIR}/move_generator.cpp
${ENGINE_SRC_DIR}/position.cpp
${ENGINE_SRC_DIR}/search.cpp
${ENGINE_SRC_DIR}/transposition_table.cpp
${ENGINE_SRC_DIR}/uci.cpp
)

set(ENGINE_SOURCE_ALL ${ENGINE_SOURCE} ${ENGINE_SOURCE_INCLUDE})