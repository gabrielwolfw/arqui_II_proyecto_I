#include "ram.hpp"
#include <stdexcept>

RAM::RAM(bool verbose) : verbose_(verbose) {
    // Initialize RAM with 512 positions of 64-bit words
    memory_.resize(RAM_SIZE, 0);
    
    if (verbose_) {
        std::cout << "RAM Initialized:\n"
                  << "- Capacity: " << RAM_SIZE << " positions × " << WORD_SIZE << " bits = "
                  << RAM_SIZE_BYTES << " bytes (" << RAM_SIZE_BYTES/1024 << " KB)\n"
                  << "- Addressing: 0x0000 to 0x" << std::hex << std::uppercase 
                  << MAX_ADDRESS << " (" << ADDRESS_BITS << " bits)\n"
                  << "- Word size: " << std::dec << WORD_SIZE << " bits (IEEE 754 double)\n";
    }
}

uint64_t RAM::read(uint32_t address) const {
    validateAddress(address);
    logOperation("READ", address, memory_[address]);
    return memory_[address];
}

void RAM::write(uint32_t address, uint64_t data) {
    validateAddress(address);
    memory_[address] = data;
    logOperation("WRITE", address, data);
}

bool RAM::isValidAddress(uint32_t address) const {
    return address <= MAX_ADDRESS;
}

void RAM::validateAddress(uint32_t address) const {
    if (!isValidAddress(address)) {
        throw std::out_of_range("Invalid memory address: 0x" + 
                               std::to_string(address) + 
                               ". Valid range is 0x0000 to 0x" + 
                               std::to_string(MAX_ADDRESS));
    }
}

void RAM::logOperation(const std::string& op, uint32_t address, uint64_t data) const {
    if (verbose_) {
        std::cout << op << " @ 0x" << std::hex << std::uppercase << std::setfill('0') 
                  << std::setw(4) << address << ": 0x" << std::setw(16) 
                  << data << std::dec << std::endl;
    }
}