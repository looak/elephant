set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)

set(ENGINE_SOURCE_INCLUDE ${ENGINE_SOURCE_INCLUDE}
${INC_DIR}/elephant_gambit.h
${INC_DIR}/defines.h
${INC_DIR}/chessboard.h
${INC_DIR}/chess_piece.h)

set(ENGINE_SOURCE ${ENGINE_SOURCE}
${SRC_DIR}/elephant_gambit.cpp
${SRC_DIR}/chessboard.cpp
${SRC_DIR}/chess_piece.cpp
${SRC_DIR}/internal_defines.h)

target_sources(${LIBRARY_NAME}
    PRIVATE
        ${ENGINE_SOURCE}
    PUBLIC
        ${ENGINE_SOURCE_INCLUDE}
)

target_include_directories(${LIBRARY_NAME}
    PUBLIC
        ${INC_DIR}
)