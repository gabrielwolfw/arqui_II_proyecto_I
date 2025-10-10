CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra
SRCDIR = src
TESTDIR = test
OBJDIR = obj

# Source files
SOURCES = $(SRCDIR)/ram/ram.cpp $(TESTDIR)/ram_test.cpp main.cpp
OBJECTS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)

# Main target
TARGET = ram_simulator

# Create necessary directories
$(shell mkdir -p $(OBJDIR)/$(SRCDIR)/ram $(OBJDIR)/$(TESTDIR))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean