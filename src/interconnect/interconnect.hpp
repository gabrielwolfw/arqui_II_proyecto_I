#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <queue>
#include <vector>
#include <memory>
#include "../bus/bus.hpp"
#include "../ram/ram.hpp"
#include "../cache/cache.hpp"  // Nuevo: incluir Cache
#include "../cache/mesi_controller.hpp"  // Nuevo: para BusEvent

class Cache;  // Forward declaration

class Interconnect {
public:
    Interconnect(std::shared_ptr<RAM> ram, bool verbose = false);
    
    // Funciones existentes
    void addRequest(const BusTransaction& transaction);
    bool processNextTransaction();
    const std::vector<BusTransaction>& getProcessedTransactions() const;
    bool hasPendingTransactions() const;
    
    // Nuevas funciones para coherencia
    void registerCache(Cache* cache);
    void broadcastBusMessage(const BusMessage& msg);
    size_t getRegisteredCacheCount() const { return caches_.size(); }

private:
    std::shared_ptr<RAM> ram_;
    std::vector<std::queue<BusTransaction>> pe_queues_;
    std::vector<BusTransaction> processed_transactions_;
    size_t current_pe_;
    bool verbose_;
    
    // Nuevo: vector de caches para coherencia
    std::vector<Cache*> caches_;
    
    size_t getNextPE();
    
    // Nuevo: helper para broadcast según tipo de evento
    void notifyCaches(uint64_t address, int sender_pe_id, BusEvent event);
};

#endif // INTERCONNECT_HPP