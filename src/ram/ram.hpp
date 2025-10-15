#ifndef RAM_HPP
#define RAM_HPP

#include <cstdint>
#include <vector>
#include <string>   // Añadido para std::string
#include <iostream> // Añadido para std::cout
#include <iomanip>  // Añadido para std::hex, std::setw, etc.

class RAM {
public:
    // Constants for RAM specifications
    static const uint32_t RAM_SIZE = 512;          // 512 positions
    static const uint32_t WORD_SIZE = 64;          // 64 bits per word
    static const uint32_t ADDRESS_BITS = 9;        // 9 bits for addressing (0x0000 to 0x01FF)
    static const uint32_t RAM_SIZE_BYTES = 4096;   // 4KB (512 * 64/8 bytes)
    static const uint32_t MAX_ADDRESS = 0x01FF;    // Maximum valid address

    RAM(bool verbose = false);
    
    // Read a 64-bit value from memory
    uint64_t read(uint32_t address) const;
    
    // Write a 64-bit value to memory
    void write(uint32_t address, uint64_t data);
    
    // Utility functions
    bool isValidAddress(uint32_t address) const;
    uint32_t getSize() const { return RAM_SIZE; }

private:
    std::vector<uint64_t> memory_;
    bool verbose_;
    
    void validateAddress(uint32_t address) const;
    void logOperation(const std::string& op, uint32_t address, uint64_t data = 0) const;
};

#endif // RAM_HPP