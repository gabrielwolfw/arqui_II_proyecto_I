#ifndef BUS_HPP
#define BUS_HPP

#include <cstdint>
#include <string>

enum class BusTransactionType {
    BusRd,    // Shared read of a block
    BusRdX,   // Read with intention to modify (exclusive read)
    BusUpgr,  // Request to transition from S → M without reloading block
    BusWB     // Write back to memory (when M block is replaced)
};

struct BusTransaction {
    BusTransactionType type;
    uint32_t address;      // Block address
    uint32_t pe_id;       // Processing Element ID
    uint32_t data;        // Data for write operations
    
    std::string toString() const {
        std::string typeStr;
        switch(type) {
            case BusTransactionType::BusRd: typeStr = "BusRd"; break;
            case BusTransactionType::BusRdX: typeStr = "BusRdX"; break;
            case BusTransactionType::BusUpgr: typeStr = "BusUpgr"; break;
            case BusTransactionType::BusWB: typeStr = "BusWB"; break;
        }
        return "PE" + std::to_string(pe_id) + " - " + typeStr + 
               " @ 0x" + std::to_string(address);
    }
};

#endif // BUS_HPP