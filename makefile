# Compiler
CXX       = mpicxx

# Release compiler/linker flags
CXXFLAGS  = -std=c++17 -Isrc -MMD -MP -fPIC -I/usr/include/nlohmann -fopenmp \
            -O3 -DNDEBUG -march=native -funroll-loops -fomit-frame-pointer -flto
LDFLAGS   = -flto -Wl,-rpath,'$$ORIGIN' -fopenmp

# Directories
SRC_DIR   = src
BUILD_DIR = build

# Gather sources
SRCS      = $(shell find $(SRC_DIR) -name '*.cpp')
SRCS_EXE  = $(wildcard $(SRC_DIR)/CLI/*.cpp)
SRCS_LIB  = $(filter-out $(SRCS_EXE), $(SRCS))

# Object files
OBJS_LIB  = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_LIB))
OBJS_EXE  = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_EXE))

# Targets
TARGET_LIB = $(BUILD_DIR)/libepanet3.so
TARGET_EXE = $(BUILD_DIR)/run-epanet3

.PHONY: all clean

# Default target
all: $(TARGET_EXE)

# Build the shared library
$(TARGET_LIB): $(OBJS_LIB)
	@mkdir -p $(dir $@)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

# Build the executable
$(TARGET_EXE): $(OBJS_EXE) $(TARGET_LIB)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $(OBJS_EXE) -L$(dir $(TARGET_LIB)) -lepanet3 $(LDFLAGS)

# Compile .cpp -> .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Cleanup
clean:
	rm -rf $(BUILD_DIR)

# Include auto-generated dependency files
-include $(OBJS_LIB:.o=.d) $(OBJS_EXE:.o=.d)
