CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

OBJS = lru_policy.o write_policy.o mesi_controller.o cache.o interconnect.o main.o

all: cache_simulator

cache_simulator: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

lru_policy.o: lru_policy.cpp lru_policy.hpp
	$(CXX) $(CXXFLAGS) -c lru_policy.cpp

write_policy.o: write_policy.cpp write_policy.hpp
	$(CXX) $(CXXFLAGS) -c write_policy.cpp

mesi_controller.o: mesi_controller.cpp mesi_controller.hpp
	$(CXX) $(CXXFLAGS) -c mesi_controller.cpp

cache.o: cache.cpp cache.hpp mesi_controller.hpp write_policy.hpp lru_policy.hpp
	$(CXX) $(CXXFLAGS) -c cache.cpp

interconnect.o: interconnect.cpp interconnect.hpp cache.hpp
	$(CXX) $(CXXFLAGS) -c interconnect.cpp

main.o: main.cpp cache.hpp interconnect.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f *.o cache_simulator

run: cache_simulator
	./cache_simulator
.PHONY: all clean