# THIS MAKEFILE IS ONLY INTENDED FOR OPENBENCH
EXE ?= elephant_ob

# Versioning
export ElephantGambit_VERSION_MAJOR = 0
export ElephantGambit_VERSION_MINOR = 10
export ElephantGambit_VERSION_PATCH = 0
export ElephantGambit_VERSION = $(ElephantGambit_VERSION_MAJOR).$(ElephantGambit_VERSION_MINOR).$(ElephantGambit_VERSION_PATCH)

# Versioning header and template
VERSION_HEADER := ../src/engine/inc/elephant_gambit_config.h
VERSION_HEADER_TEMPLATE := ../src/engine/inc/elephant_gambit_config.h.in

# Directories
INCLUDE_PATHS := ../src/engine/inc ../src/cli/inc ../third_party/spdlog/include
INC_DIR := $(addprefix -I,$(INCLUDE_PATHS))
CSRC_DIR := ../src/cli/src
ESRC_DIR := ../src/engine/src
BUILD_DIR := ./build

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++2a -O3 -flto=auto -march=native -MMD -MP $(INC_DIR) -Wall -Wextra -Wpedantic -Wshadow -Wsign-conversion -Wno-unknown-pragmas -Wunused-variable -Wunused-parameter -Wconversion

# Source files
ENGINE_SRC := $(shell find $(ESRC_DIR) -name "*.cpp")
COMMAND_SRC := $(CSRC_DIR)/commands/uci_commands.cpp $(CSRC_DIR)/commands/command_processor.cpp
SRC := $(CSRC_DIR)/openbench.cpp $(COMMAND_SRC) $(ENGINE_SRC)

# Object files
OBJ := $(addprefix $(BUILD_DIR)/, $(notdir $(patsubst %.cpp,%.o,$(SRC))))
DEP := $(OBJ:.o=.d)

# Executable in bin directory
EXE_PATH := $(EXE)

# Phony targets
.PHONY: all clean

vpath %.cpp $(CSRC_DIR) $(CSRC_DIR)/commands $(shell find $(ESRC_DIR) -type d)

# Main target
all: $(EXE_PATH)

# Create directories if they don't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Executable dependency on object files AND version header
$(EXE_PATH): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Object file compilation rule
$(BUILD_DIR)/%.o: %.cpp $(VERSION_HEADER) | $(BUILD_DIR)
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
	rm -rf $(BUILD_DIR) $(EXE_PATH) $(VERSION_HEADER)

# Debug rule
print-%:
	@echo $* = $($*)
