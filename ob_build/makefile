# THIS MAKEFILE IS ONLY INTENDED FOR OPENBENCH
EXE ?= elephant_ob

# Source, Header, and Dependency file locations
SOURCES := ../src/cli/src/commands_uci.cpp ../src/cli/src/openbench.cpp ../src/engine/src/bitboard.cpp ../src/engine/src/chess_piece.cpp ../src/engine/src/chessboard.cpp ../src/engine/src/clock.cpp ../src/engine/src/elephant_gambit.cpp ../src/engine/src/evaluator.cpp ../src/engine/src/fen_parser.cpp ../src/engine/src/game_context.cpp ../src/engine/src/hash_zorbist.cpp ../src/engine/src/king_pin_threats.cpp ../src/engine/src/log.cpp ../src/engine/src/material_mask.cpp ../src/engine/src/move.cpp ../src/engine/src/move_generator.cpp ../src/engine/src/notation.cpp ../src/engine/src/position.cpp ../src/engine/src/search.cpp ../src/engine/src/transposition_table.cpp ../src/engine/src/uci.cpp 
HEADERS := ../src/engine/inc/bitboard.hpp ../src/engine/inc/bitboard_constants.hpp ../src/engine/inc/chess_piece.h ../src/engine/inc/chess_piece_defines.hpp ../src/engine/inc/chessboard.h ../src/engine/inc/clock.hpp ../src/engine/inc/defines.h ../src/engine/inc/elephant_gambit.h ../src/engine/src/evaluator_data.h ../src/engine/inc/evaluation_table.hpp ../src/engine/inc/evaluator.h ../src/engine/inc/fen_parser.h ../src/engine/inc/game_context.h ../src/engine/inc/hash_zorbist.h ../src/engine/inc/intrinsics.hpp ../src/engine/inc/king_pin_threats.hpp ../src/engine/inc/libpopcnt.h ../src/engine/inc/log.h ../src/engine/inc/material_mask.hpp ../src/engine/inc/move.h ../src/engine/inc/move_generator.hpp ../src/engine/inc/notation.h ../src/engine/inc/position.hpp ../src/engine/inc/search.hpp ../src/engine/inc/search_constants.hpp ../src/engine/inc/transposition_table.hpp ../src/engine/inc/uci.hpp
INCDIR := ../src/engine/inc/

# Output directories for object and dependency files
OBJDIR := out

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++2a -O3 -flto -march=native -I$(INCDIR) -MMD -MP

# Extract the basenames of source files
SOURCES_BASENAMES := $(notdir $(SOURCES))

# Create object file list, now with directory
OBJECTS := $(addprefix $(OBJDIR)/,$(SOURCES_BASENAMES:.cpp=.o)) # Add prefix to create full path

# Executable output file
OUT := $(EXE)

# Default target: build the executable
all: $(OUT)

# Link object files to create the executable
$(OUT): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ensure the output directory exists
$(OBJDIR):
	mkdir -p $@

# Compilation rule for .cpp files
$(OBJDIR)/%.o: ../src/cli/src/%.cpp $(HEADERS)	
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: ../src/engine/src/%.cpp $(HEADERS)	
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Include dependency files (using basenames)
-include $(patsubst %,$(DEPDIR)/%.d,$(SOURCES_BASENAMES))

# Clean target (now includes directories)
clean:
	rm -f $(OUT) $(OBJECTS) $(OBJDIR)/*.d