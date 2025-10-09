# ===========================================
# Makefile - Proyecto PEs + ISA modular
# ===========================================

TARGET_DIR = ../bin
TARGET = $(TARGET_DIR)/pe_tests

CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

LOADER_DIR = Loader
PE_DIR = PE
INSTR_DIR = Instructions
MEM_DIR = Memory

INCLUDES = -I$(LOADER_DIR) -I$(PE_DIR) -I$(INSTR_DIR) -I$(MEM_DIR)

SRC = $(LOADER_DIR)/Loader.cpp \
      $(PE_DIR)/PE.cpp \
      $(MEM_DIR)/MockMemPort.cpp \
      main.cpp

HEADERS = $(LOADER_DIR)/Loader.hpp \
          $(PE_DIR)/PE.hpp \
          $(INSTR_DIR)/Instruction.hpp \
          $(MEM_DIR)/IMemPort.hpp \
          $(MEM_DIR)/MockMemPort.hpp

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	mkdir -p $(TARGET_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)
	@echo "✅ Compilación completada."
	@echo "Ejecuta con: ./$(TARGET)"

run: all
	./$(TARGET)

clean:
	rm -rf $(TARGET_DIR)
	@echo "🧹 Carpeta bin eliminada."
