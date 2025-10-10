#ifndef RAM_TEST_HPP
#define RAM_TEST_HPP

#include "../src/ram/ram.hpp"
#include <cassert>

class RAMTest {
public:
    /**
     * @brief Run all RAM tests
     * @param verbose Enable verbose output
     * @return true if all tests pass, false otherwise
     */
    static bool runTests(bool verbose = false);

private:
    static bool testWriteRead(RAM& ram);
    static bool testInvalidAddress(RAM& ram);
    static bool testBoundaryValues(RAM& ram);
};

#endif // RAM_TEST_HPP