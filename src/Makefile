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
CACHE_DIR = cache
INTERCONNECT_DIR = interconect_temp

INCLUDES = -I$(LOADER_DIR) -I$(PE_DIR) -I$(INSTR_DIR) -I$(MEM_DIR) -I$(CACHE_DIR) -I$(INTERCONNECT_DIR)

SRC = $(LOADER_DIR)/Loader.cpp \
      $(PE_DIR)/PE.cpp \
      $(MEM_DIR)/MockMemPort.cpp \
      $(CACHE_DIR)/cache.cpp \
      $(CACHE_DIR)/lru_policy.cpp \
      $(CACHE_DIR)/mesi_controller.cpp \
      $(CACHE_DIR)/write_policy.cpp \
      main.cpp

HEADERS = $(LOADER_DIR)/Loader.hpp \
          $(PE_DIR)/PE.hpp \
          $(INSTR_DIR)/Instruction.hpp \
          $(MEM_DIR)/IMemPort.hpp \
          $(MEM_DIR)/MockMemPort.hpp \
          $(CACHE_DIR)/cache.hpp \
          $(CACHE_DIR)/lru_policy.hpp \
          $(CACHE_DIR)/mesi_controller.hpp \
          $(CACHE_DIR)/write_policy.hpp \
          

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	mkdir -p $(TARGET_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)
	@echo "Compilación completada."
	@echo "Ejecuta con: ./$(TARGET)"

run: all
	./$(TARGET)

clean:
	rm -rf $(TARGET_DIR)
	@echo "Carpeta bin eliminada."