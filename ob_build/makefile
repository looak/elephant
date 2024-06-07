# THIS MAKEFILE IS ONLY INTENDED FOR OPENBENCH
EXE ?= elephant_ob

# Versioning
export ElephantGambit_VERSION_MAJOR = 0
export ElephantGambit_VERSION_MINOR = 6
export ElephantGambit_VERSION_PATCH = 5
export ElephantGambit_VERSION = $(ElephantGambit_VERSION_MAJOR).$(ElephantGambit_VERSION_MINOR).$(ElephantGambit_VERSION_PATCH)

# Directories
CSRC_DIR := ../src/cli/src
ESRC_DIR := ../src/engine/src
INC_DIR := ../src/engine/inc 



# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++2a -O3 -flto -march=native -MMD -MP -I$(INC_DIR)

# Versioning header and template
VERSION_HEADER := ../src/engine/inc/elephant_gambit_config.h
VERSION_HEADER_TEMPLATE := ../src/engine/inc/elephant_gambit_config.h.in

# Source files
ENGINE_SRC := $(wildcard $(ESRC_DIR)/*.cpp)
COMMAND_SRC := $(CSRC_DIR)/commands_uci.cpp
SRC := $(CSRC_DIR)/openbench.cpp $(COMMAND_SRC) $(ENGINE_SRC)

# Object files
OBJ := $(patsubst %.cpp,%.o,$(SRC))
DEP := $(OBJ:.o=.d)

# Phony targets
.PHONY: all clean

# Main target
all: $(EXE)

# Executable dependency on object files AND version header
$(EXE): $(OBJ) 
	$(CXX) $(CXXFLAGS) -o $@ $^

# Object file compilation rule
%.o: %.cpp $(VERSION_HEADER)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Version header generation rule (simplified for readability)
$(VERSION_HEADER): $(VERSION_HEADER_TEMPLATE)
	sed -e 's/@ElephantGambit_VERSION_MAJOR@/$(ElephantGambit_VERSION_MAJOR)/g' \
		-e 's/@ElephantGambit_VERSION_MINOR@/$(ElephantGambit_VERSION_MINOR)/g' \
		-e 's/@ElephantGambit_VERSION_PATCH@/$(ElephantGambit_VERSION_PATCH)/g' \
		-e 's/@ElephantGambit_VERSION@/$(ElephantGambit_VERSION)/g' \
		$< > $@

# Clean rule
clean:
	rm -f $(EXE) $(OBJ) $(VERSION_HEADER) $(DEP)
