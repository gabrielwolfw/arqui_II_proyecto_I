#ifndef RAM_TEST_HPP
#define RAM_TEST_HPP

#include "../../src/ram/ram.hpp"
#include <iostream>
#include <cstdint>

class RAMTest {
public:
    static bool runAllTests() {
        bool success = true;
        RAM ram(true);  // Create RAM with verbose output

        std::cout << "\nStarting RAM tests...\n";

        success &= testWriteRead(ram);
        success &= testInvalidAddress(ram);
        success &= testBoundaryValues(ram);

        if (success) {
            std::cout << "All RAM tests passed!\n";
        } else {
            std::cout << "Some RAM tests failed!\n";
        }

        return success;
    }

private:
    static bool testWriteRead(RAM& ram);
    static bool testInvalidAddress(RAM& ram);
    static bool testBoundaryValues(RAM& ram);
};

#endif // RAM_TEST_HPP