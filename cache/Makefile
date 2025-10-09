CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
TARGET = cache_simulator

# Archivos objeto
OBJS = lru_policy.o \
       mesi_controller.o \
       write_policy.o \
       cache.o \
       test_cache.o

# Regla principal
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Reglas individuales
lru_policy.o: lru_policy.cpp lru_policy.h
	$(CXX) $(CXXFLAGS) -c lru_policy.cpp

mesi_controller.o: mesi_controller.cpp mesi_controller.h
	$(CXX) $(CXXFLAGS) -c mesi_controller.cpp

write_policy.o: write_policy.cpp write_policy.h
	$(CXX) $(CXXFLAGS) -c write_policy.cpp

cache.o: cache.cpp cache.h lru_policy.h mesi_controller.h write_policy.h
	$(CXX) $(CXXFLAGS) -c cache.cpp

test_cache.o: test_cache.cpp cache.h
	$(CXX) $(CXXFLAGS) -c test_cache.cpp

# Limpiar archivos compilados
clean:
	rm -f $(OBJS) $(TARGET)

# Compilar y ejecutar
run: all
	./$(TARGET)

.PHONY: all clean run