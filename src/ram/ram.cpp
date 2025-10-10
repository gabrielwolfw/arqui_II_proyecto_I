#include "ram.hpp"

RAM::RAM(bool verbose) : verbose(verbose) {
    clear();
}

bool RAM::write(uint16_t address, double value) {
    if (!isValidAddress(address)) {
        if (verbose) {
            std::cerr << "Error: Invalid write address 0x" 
                      << std::hex << address << std::endl;
        }
        return false;
    }

    memory[address] = value;
    
    if (verbose) {
        std::cout << "Write to address 0x" << std::hex << address 
                  << ": " << std::dec << value << std::endl;
    }
    
    return true;
}

double RAM::read(uint16_t address, bool* success) {
    if (!isValidAddress(address)) {
        if (verbose) {
            std::cerr << "Error: Invalid read address 0x" 
                      << std::hex << address << std::endl;
        }
        if (success) *success = false;
        return 0.0;
    }

    if (success) *success = true;
    
    if (verbose) {
        std::cout << "Read from address 0x" << std::hex << address 
                  << ": " << std::dec << memory[address] << std::endl;
    }
    
    return memory[address];
}

bool RAM::isValidAddress(uint16_t address) const {
    return address < RAM_SIZE;
}

void RAM::clear() {
    memory.fill(0.0);
    if (verbose) {
        std::cout << "Memory cleared" << std::endl;
    }
}