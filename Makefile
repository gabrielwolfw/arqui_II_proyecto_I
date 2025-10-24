# Compiler settings
CXX = g++
CXXFLAGS = -Wall -std=c++11 -I.

# Directories
SRC_DIR = src
OBJ_DIR = obj
TEST_DIR = test

# Source files
RAM_SRCS = $(SRC_DIR)/ram/ram.cpp
INTERCONNECT_SRCS = $(SRC_DIR)/interconnect/interconnect.cpp
TEST_SRCS = $(TEST_DIR)/ram/ram_test.cpp $(TEST_DIR)/interconnect/interconnect_test.cpp

# Object files
RAM_OBJS = $(RAM_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/src/%.o)
INTERCONNECT_OBJS = $(INTERCONNECT_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/src/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.cpp=$(OBJ_DIR)/test/%.o)
MAIN_OBJ = $(OBJ_DIR)/main.o

# Main target
TARGET = simulator

# Phony targets
.PHONY: all clean directories

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR)/src/ram
	@mkdir -p $(OBJ_DIR)/src/interconnect
	@mkdir -p $(OBJ_DIR)/test/ram
	@mkdir -p $(OBJ_DIR)/test/interconnect

# Main executable
$(TARGET): $(RAM_OBJS) $(INTERCONNECT_OBJS) $(TEST_OBJS) $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compile main
$(OBJ_DIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile RAM source files
$(OBJ_DIR)/src/ram/%.o: $(SRC_DIR)/ram/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile Interconnect source files
$(OBJ_DIR)/src/interconnect/%.o: $(SRC_DIR)/interconnect/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile test files
$(OBJ_DIR)/test/ram/%.o: $(TEST_DIR)/ram/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/test/interconnect/%.o: $(TEST_DIR)/interconnect/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)