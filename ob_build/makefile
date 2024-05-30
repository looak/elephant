# THIS MAKEFILE IS ONLY INTENDED FOR OPENBENCH
EXE ?= elephant_ob

SOURCES := ../src/cli/src/commands_uci.cpp ../src/cli/src/openbench.cpp ../src/engine/src/bitboard.cpp ../src/engine/src/chess_piece.cpp ../src/engine/src/chessboard.cpp ../src/engine/src/clock.cpp ../src/engine/src/elephant_gambit.cpp ../src/engine/src/evaluator.cpp ../src/engine/src/evaluator_data.h ../src/engine/src/fen_parser.cpp ../src/engine/src/game_context.cpp ../src/engine/src/hash_zorbist.cpp ../src/engine/src/internal_defines.h ../src/engine/src/king_pin_threats.cpp ../src/engine/src/log.cpp ../src/engine/src/material_mask.cpp ../src/engine/src/move.cpp ../src/engine/src/move_generator.cpp ../src/engine/src/notation.cpp ../src/engine/src/position.cpp ../src/engine/src/search.cpp ../src/engine/src/transposition_table.cpp ../src/engine/src/uci.cpp ../src/engine/inc/bitboard.hpp ../src/engine/inc/bitboard_constants.h ../src/engine/inc/chess_piece.h ../src/engine/inc/chess_piece_defines.hpp ../src/engine/inc/chessboard.h ../src/engine/inc/clock.hpp ../src/engine/inc/defines.h ../src/engine/inc/elephant_gambit.h ../src/engine/inc/evaluation_table.hpp ../src/engine/inc/evaluator.h ../src/engine/inc/fen_parser.h ../src/engine/inc/game_context.h ../src/engine/inc/hash_zorbist.h ../src/engine/inc/intrinsics.hpp ../src/engine/inc/king_pin_threats.hpp ../src/engine/inc/libpopcnt.h ../src/engine/inc/log.h ../src/engine/inc/material_mask.hpp ../src/engine/inc/move.h ../src/engine/inc/move_generator.hpp ../src/engine/inc/notation.h ../src/engine/inc/position.hpp ../src/engine/inc/search.hpp ../src/engine/inc/search_constants.hpp ../src/engine/inc/transposition_table.hpp ../src/engine/inc/uci.hpp 
DEPS := ../src/engine/inc/

CXX := g++
CXXFLAGS := -std=c++2a -O3 -flto -march=native -I$(DEPS)


OUT := $(EXE)

all: $(EXE)

$(EXE): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(OUT) $^

clean:
	rm -f $(OUT)