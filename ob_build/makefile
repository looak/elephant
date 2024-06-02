# THIS MAKEFILE IS ONLY INTENDED FOR OPENBENCH
EXE ?= elephant_ob

# Versioning
export ElephantGambit_VERSION_MAJOR = 0
export ElephantGambit_VERSION_MINOR = 6
export ElephantGambit_VERSION_PATCH = 5
export ElephantGambit_VERSION = $(ElephantGambit_VERSION_MAJOR).$(ElephantGambit_VERSION_MINOR).$(ElephantGambit_VERSION_PATCH)

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++2a -O3 -flto -march=native -MMD -MP

# Source files
CSRC_DIR := ../src/cli/src
ESRC_DIR := ../src/engine/src
INC_DIR := ../src/engine/inc 

ENGINE_SRC := $(wildcard $(ESRC_DIR)/*.cpp)
COMMAND_SRC := $(CSRC_DIR)/commands_uci.cpp

ENGINE_INC := $(wildcard $(INC_DIR)/*.hpp)
ENGINE_INC += $(wildcard $(INC_DIR)/*.h)

SRC := $(CSRC_DIR)/openbench.cpp $(COMMAND_SRC) $(ENGINE_SRC)

CXXFLAGS += -I$(INC_DIR)

all: $(EXE)

# Versioning header
$(INC_DIR)/elephant_gambit_config.h: $(INC_DIR)/elephant_gambit_config.h.in
	sed \
		-e 's/@ElephantGambit_VERSION_MAJOR@/$(ElephantGambit_VERSION_MAJOR)/g' \
		-e 's/@ElephantGambit_VERSION_MINOR@/$(ElephantGambit_VERSION_MINOR)/g' \
		-e 's/@ElephantGambit_VERSION_PATCH@/$(ElephantGambit_VERSION_PATCH)/g' \
		-e 's/@ElephantGambit_VERSION@/$(ElephantGambit_VERSION)/g' \
		-e 's/@ElephantGambit_VERSION_SUFFIX@/$(ElephantGambit_VERSION_SUFFIX)/g' \
		-e 's/@ElephantGambit_VERSION_PRERELEASE@/$(ElephantGambit_VERSION_PRERELEASE)/g' \
		$< > $@

$(EXE): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(EXE) *.o *.d