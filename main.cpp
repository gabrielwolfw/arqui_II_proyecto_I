#include <iostream>
#include "src/interconnect/interconnect.hpp"
#include "src/ram/ram.hpp"

// Forward declarations of test functions
void test_round_robin();
void test_memory_operations();

int main() {
    std::cout << "Starting Interconnect Tests..." << std::endl;
    
    try {
        test_round_robin();
        test_memory_operations();
        std::cout << "All tests passed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}