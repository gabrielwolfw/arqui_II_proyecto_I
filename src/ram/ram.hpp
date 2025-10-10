#ifndef RAM_HPP
#define RAM_HPP

#include <array>
#include <cstdint>
#include <iostream>

class RAM {
public:
    static constexpr size_t RAM_SIZE = 512;  // 512 positions
    static constexpr size_t ADDR_BITS = 9;   // 9 bits for addressing

private:
    std::array<double, RAM_SIZE> memory;
    bool verbose;

public:
    RAM(bool verbose = false);
    
    /**
     * @brief Write a double value to the specified memory address
     * @param address Memory address (0x0000 to 0x01FF)
     * @param value Double value to write (64-bit IEEE 754)
     * @return true if write was successful, false if address is invalid
     */
    bool write(uint16_t address, double value);

    /**
     * @brief Read a double value from the specified memory address
     * @param address Memory address (0x0000 to 0x01FF)
     * @param success Pointer to bool that will be set to indicate read success
     * @return The value read from memory, 0.0 if address is invalid
     */
    double read(uint16_t address, bool* success = nullptr);

    /**
     * @brief Check if an address is valid
     * @param address Memory address to check
     * @return true if address is valid, false otherwise
     */
    bool isValidAddress(uint16_t address) const;

    /**
     * @brief Clear all memory positions to 0.0
     */
    void clear();
};

#endif // RAM_HPP