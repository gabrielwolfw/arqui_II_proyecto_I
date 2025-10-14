# Carpetas
BIN_DIR = ../../bin
BUILD_DIR = ../../build/cache

# Archivos fuente
SRCS = cache.cpp interconnect.cpp lru_policy.cpp mesi_controller.cpp write_policy.cpp main.cpp
HDRS = cache.hpp interconnect.hpp lru_policy.hpp mesi_controller.hpp write_policy.hpp bus_interface.hpp
OBJS = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

# Nombre del ejecutable
TARGET = $(BIN_DIR)/cache_simulator

# Compilador y flags
CXX = g++
CXXFLAGS = -Wall -I. # -I. para incluir los headers locales

# Regla por defecto
all: $(TARGET)

# Regla para crear el binario
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Regla para compilar archivos objeto en build/cache/
$(BUILD_DIR)/%.o: %.cpp $(HDRS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar objetos y binario
clean:
	rm -rf $(BUILD_DIR)/*.o $(TARGET)

.PHONY: all clean