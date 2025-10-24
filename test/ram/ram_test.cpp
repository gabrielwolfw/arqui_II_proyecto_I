#include "ram_test.hpp"
#include <cassert>

bool RAMTest::testWriteRead(RAM& ram) {
    std::cout << "Testing write and read operations...\n";
    bool success = true;
    
    // Test basic write/read operations
    uint32_t testAddr = 0x0100;
    uint64_t testValue = 0xCAFEBABE;
    
    try {
        // Reset memory to known state
        ram.write(testAddr, 0);
        
        // Write test value
        ram.write(testAddr, testValue);
        
        // Read and verify
        uint64_t readValue = ram.read(testAddr);
        success &= (readValue == testValue);
        
        if (success) {
            std::cout << "Write/Read test passed!\n";
        } else {
            std::cout << "Write/Read test failed! Expected: " << testValue 
                      << ", Got: " << readValue << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "Write/Read test failed with exception: " << e.what() << "\n";
        success = false;
    }
    
    return success;
}

bool RAMTest::testInvalidAddress(RAM& ram) {
    std::cout << "Testing invalid address handling...\n";
    bool success = true;
    
    try {
        // Try to write to invalid address
        ram.write(RAM::MAX_ADDRESS + 1, 1);
        std::cout << "Invalid address test failed: Should have thrown exception\n";
        success = false;
    } catch (const std::out_of_range&) {
        std::cout << "Invalid address write correctly rejected\n";
    }
    
    try {
        // Try to read from invalid address
        ram.read(RAM::MAX_ADDRESS + 1);
        std::cout << "Invalid address test failed: Should have thrown exception\n";
        success = false;
    } catch (const std::out_of_range&) {
        std::cout << "Invalid address read correctly rejected\n";
    }
    
    return success;
}

bool RAMTest::testBoundaryValues(RAM& ram) {
    std::cout << "Testing boundary values...\n";
    bool success = true;
    uint64_t testValue = 0xDEADBEEF;
    
    try {
        // Test first address
        ram.write(0x0000, testValue);
        success &= (ram.read(0x0000) == testValue);
        
        // Test last valid address
        ram.write(RAM::MAX_ADDRESS, testValue);
        success &= (ram.read(RAM::MAX_ADDRESS) == testValue);
        
        if (success) {
            std::cout << "Boundary values test passed!\n";
        } else {
            std::cout << "Boundary values test failed!\n";
        }
    } catch (const std::exception& e) {
        std::cout << "Boundary values test failed with exception: " << e.what() << "\n";
        success = false;
    }
    
    return success;
}