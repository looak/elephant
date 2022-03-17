set(ENGINE_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(ENGINE_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)

set(ENGINE_SOURCE_INCLUDE ${ENGINE_SOURCE_INCLUDE}
${ENGINE_INC_DIR}/elephant_gambit.h
${ENGINE_INC_DIR}/defines.h
${ENGINE_INC_DIR}/chessboard.h
${ENGINE_INC_DIR}/chess_piece.h
${ENGINE_INC_DIR}/fen_parser.h
${ENGINE_INC_DIR}/game_context.h
${ENGINE_INC_DIR}/bitboard.h
${ENGINE_INC_DIR}/log.h
${ENGINE_INC_DIR}/move.h)


set(ENGINE_SOURCE ${ENGINE_SOURCE}
${ENGINE_SRC_DIR}/elephant_gambit.cpp
${ENGINE_SRC_DIR}/chessboard.cpp
${ENGINE_SRC_DIR}/chess_piece.cpp
${ENGINE_SRC_DIR}/fen_parser.cpp
${ENGINE_SRC_DIR}/internal_defines.h
${ENGINE_SRC_DIR}/bitboard.cpp
${ENGINE_SRC_DIR}/move.cpp)
