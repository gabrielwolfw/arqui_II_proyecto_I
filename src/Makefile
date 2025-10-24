TARGET_DIR = ../bin
TARGET = $(TARGET_DIR)/pe_tests

CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

# Directorios del proyecto src/
LOADER_DIR = Loader
PE_DIR = PE
INSTR_DIR = Instructions
MEM_DIR = Memory
CACHE_DIR = cache

# Includes: agregar rutas a interconnect y ram
INCLUDES = -I$(LOADER_DIR) -I$(PE_DIR) -I$(INSTR_DIR) -I$(MEM_DIR) \
           -I$(CACHE_DIR) -I../src/bus -I../src/ram -I../src/interconnect

# Archivos fuente
SRC = $(LOADER_DIR)/Loader.cpp \
      $(PE_DIR)/PE.cpp \
      $(MEM_DIR)/MockMemPort.cpp \
      $(CACHE_DIR)/cache.cpp \
      $(CACHE_DIR)/lru_policy.cpp \
      $(CACHE_DIR)/mesi_controller.cpp \
      $(CACHE_DIR)/write_policy.cpp \
      ../src/ram/ram.cpp \
      ../src/interconnect/interconnect.cpp \
      main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p $(TARGET_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)
	@echo "✓ Compilación completada"
	@echo "Ejecuta con: ./$(TARGET)"

run: all
	./$(TARGET)

clean:
	rm -rf $(TARGET_DIR)
	rm -f *.o
	@echo "Limpieza completada"

.PHONY: all run clean