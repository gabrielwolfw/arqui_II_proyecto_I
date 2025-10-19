CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

TARGET = dot_product_test
SRCS = dot_product_test.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)