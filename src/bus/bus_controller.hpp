#ifndef BUS_CONTROLLER_HPP
#define BUS_CONTROLLER_HPP

#include <vector>
#include <cstdint>

// Forward declaration
class Cache;

class BusController {
    std::vector<Cache*> caches;
    bool verbose;
    
public:
    BusController(bool verbose = false) : verbose(verbose) {}
    
    void registerCache(Cache* cache) {
        caches.push_back(cache);
    }
    
    // Notifica a todas las cachés sobre una transacción en el bus
    void notifyTransaction(uint64_t address, int originating_pe_id) {
        // Aquí puedes implementar la lógica de snooping
        // Por ahora es un placeholder para coherencia MESI
        if (verbose) {
            std::cout << "[BusController] Broadcasting transaction for addr=0x" 
                      << std::hex << address << std::dec 
                      << " from PE" << originating_pe_id << std::endl;
        }
    }
    
    size_t getCacheCount() const {
        return caches.size();
    }
};

#endif // BUS_CONTROLLER_HPP