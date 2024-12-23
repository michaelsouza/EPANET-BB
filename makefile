# =============================================================================
# Makefile for Epanet Project
# Supports Debug and Release build modes
# =============================================================================

# -------------------------------
# Compiler and Tools
# -------------------------------
CC      := gcc
AR      := ar
RM      := rm -rf

# -------------------------------
# Directories
# -------------------------------
INCLUDE_DIRS    := -Iinclude -Isrc -Isrc/include -Isrc/outfile/include -Isrc/util
SRC_DIR         := src
OUTFILE_SRC_DIR := src/outfile/src
UTIL_DIR        := src/util
RUN_DIR         := run
BUILD_DIR       := build

# -------------------------------
# Build Modes
# -------------------------------
# Default build mode is 'release'
MODE ?= release

ifeq ($(MODE),debug)
    CFLAGS          := -Wall -g -O0 -DDEBUG $(INCLUDE_DIRS)
    EXECUTABLE      := main_debug
    BUILD_SUBDIR    := debug
else ifeq ($(MODE),release)
    CFLAGS          := -Wall -O2 $(INCLUDE_DIRS)
    EXECUTABLE      := main
    BUILD_SUBDIR    := release
else
    $(error Unknown MODE: $(MODE). Use 'debug' or 'release'.)
endif

# Linker Flags (Add libraries here if needed)
LDFLAGS         := -lm  # Added -lm to link the math library

# -------------------------------
# Source Files
# -------------------------------
SRC_FILES := \
    $(SRC_DIR)/epanet2.c \
    $(SRC_DIR)/epanet.c \
    $(SRC_DIR)/flowbalance.c \
    $(SRC_DIR)/genmmd.c \
    $(SRC_DIR)/hash.c \
    $(SRC_DIR)/hydcoeffs.c \
    $(SRC_DIR)/hydraul.c \
    $(SRC_DIR)/hydsolver.c \
    $(SRC_DIR)/hydstatus.c \
    $(SRC_DIR)/inpfile.c \
    $(SRC_DIR)/input1.c \
    $(SRC_DIR)/input2.c \
    $(SRC_DIR)/input3.c \
    $(SRC_DIR)/leakage.c \
    $(SRC_DIR)/mempool.c \
    $(SRC_DIR)/output.c \
    $(SRC_DIR)/project.c \
    $(SRC_DIR)/quality.c \
    $(SRC_DIR)/qualreact.c \
    $(SRC_DIR)/qualroute.c \
    $(SRC_DIR)/report.c \
    $(SRC_DIR)/rules.c \
    $(SRC_DIR)/smatrix.c \
    $(SRC_DIR)/validate.c \
    $(OUTFILE_SRC_DIR)/epanet_output.c \
    $(UTIL_DIR)/cstr_helper.c \
    $(UTIL_DIR)/errormanager.c \
    $(UTIL_DIR)/filemanager.c \
    $(RUN_DIR)/main.c

# -------------------------------
# Object Files
# -------------------------------
# Replace .c with .o and prefix with build directory
OBJ_FILES := $(patsubst %.c,$(BUILD_DIR)/$(BUILD_SUBDIR)/%.o,$(SRC_FILES))

# -------------------------------
# Targets
# -------------------------------

# Default target
all: $(BUILD_DIR)/$(BUILD_SUBDIR)/$(EXECUTABLE)

# Link the executable
$(BUILD_DIR)/$(BUILD_SUBDIR)/$(EXECUTABLE): $(OBJ_FILES)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Linked executable: $@"

# Compile source files to object files
$(BUILD_DIR)/$(BUILD_SUBDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled: $< -> $@"

# Phony Targets for build modes
debug: MODE=debug
debug: clean all
	@echo "Built in DEBUG mode."

release: MODE=release
release: clean all
	@echo "Built in RELEASE mode."

# Clean the build directories
clean:
	$(RM) $(BUILD_DIR)
	@echo "Cleaned build directories."

# -------------------------------
# Phony Declarations
# -------------------------------
.PHONY: all clean debug release
