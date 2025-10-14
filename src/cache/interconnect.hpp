#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include "bus_interface.hpp"
#include "cache.hpp"
#include <vector>
#include <queue>

class Interconnect : public IBusInterface {
private:
    std::vector<Cache*> caches;
    std::queue<BusMessage> message_queue;
    
public:
    Interconnect() = default;
    
    // Registrar un PE en el interconnect
    void registerCache(Cache* cache);
    
    // Implementación de IBusInterface
    void sendMessage(const BusMessage& msg) override;
    void readFromMemory(uint64_t address, uint8_t* data, size_t size) override;
    void writeToMemory(uint64_t address, const uint8_t* data, size_t size) override;
    void supplyData(uint64_t address, const uint8_t* data) override;
    
    // Métodos específicos del interconnect
    void processMessages();
    size_t getConnectedPECount() const;
    void clearMessages();
};

#endif // INTERCONNECT_H