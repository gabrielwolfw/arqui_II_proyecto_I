#include "ram_test.hpp"
#include <iostream>
#include <limits>

bool RAMTest::runTests(bool verbose) {
    RAM ram(verbose);
    bool success = true;

    std::cout << "Running RAM tests..." << std::endl;

    success &= testWriteRead(ram);
    success &= testInvalidAddress(ram);
    success &= testBoundaryValues(ram);

    if (success) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << "Some tests failed!" << std::endl;
    }

    return success;
}

bool RAMTest::testWriteRead(RAM& ram) {
    std::cout << "Testing basic write/read operations..." << std::endl;
    
    bool success = true;
    ram.clear();

    // Test some regular values
    double testValue = 123.456;
    uint16_t testAddr = 0x0100;

    success &= ram.write(testAddr, testValue);
    testValue = 493.4551;
    testAddr = 0x0100;

    success &= ram.write(testAddr, testValue);
    bool readSuccess;
    double readValue = ram.read(testAddr, &readSuccess);
    
    success &= readSuccess;
    success &= (readValue == testValue);

    return success;
}

bool RAMTest::testInvalidAddress(RAM& ram) {
    std::cout << "Testing invalid addresses..." << std::endl;
    
    bool success = true;
    
    // Test address outside valid range
    success &= !ram.write(0x0200, 1.0);  // Should fail
    bool readSuccess;
    ram.read(0x0200, &readSuccess);
    success &= !readSuccess;  // Should fail

    return success;
}

bool RAMTest::testBoundaryValues(RAM& ram) {
    std::cout << "Testing boundary values..." << std::endl;
    
    bool success = true;
    
    // Test minimum and maximum valid addresses
    success &= ram.write(0x0000, 1.0);  // First address
    success &= ram.write(0x01FF, 1.0);  // Last address

    return success;
}