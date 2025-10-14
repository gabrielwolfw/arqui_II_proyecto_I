#include "interconnect.hpp"
#include <iostream>
#include <cstring>

void Interconnect::registerCache(Cache* cache) {
    caches.push_back(cache);
}

void Interconnect::sendMessage(const BusMessage& msg) {
    message_queue.push(msg);
}

void Interconnect::processMessages() {
    while (!message_queue.empty()) {
        BusMessage msg = message_queue.front();
        message_queue.pop();
        
        std::cout << "[Interconnect] Broadcasting message from PE" 
                  << msg.sender_pe_id << " for addr=0x" 
                  << std::hex << msg.address << std::dec << std::endl;
        
        // Broadcast a todos los PEs excepto el emisor
        for (size_t i = 0; i < caches.size(); i++) {
            Cache* cache = caches[i];
            if (cache != nullptr) {
                // Los otros PEs reaccionan según el evento
                switch (msg.event) {
                    case BusEvent::BUS_READ:
                        cache->handleBusRead(msg.address);
                        break;
                        
                    case BusEvent::BUS_READX:
                        cache->handleBusReadX(msg.address);
                        break;
                        
                    case BusEvent::BUS_UPGRADE:
                        // BUS_UPGRADE invalida otras copias en estado S
                        cache->invalidateLine(msg.address);
                        break;
                        
                    default:
                        break;
                }
            }
        }
    }
}

void Interconnect::writeToMemory(uint64_t address, const uint8_t* data, size_t size) {
    (void)data;  // Suprimir warning
    std::cout << "[Interconnect] Writing to MEMORY: addr=0x" 
              << std::hex << address << std::dec 
              << " size=" << size << " bytes" << std::endl;
    // TODO: Implementar escritura real a memoria
}

void Interconnect::readFromMemory(uint64_t address, uint8_t* data, size_t size) {
    std::cout << "[Interconnect] Reading from MEMORY: addr=0x" 
              << std::hex << address << std::dec 
              << " size=" << size << " bytes" << std::endl;
    // Por ahora llena con datos dummy
    std::memset(data, 0xCD, size);
}

void Interconnect::supplyData(uint64_t address, const uint8_t* data) {
    (void)data;  // Suprimir warning
    std::cout << "[Interconnect] Cache-to-cache transfer for addr=0x" 
              << std::hex << address << std::dec << std::endl;
    // TODO: Implementar transferencia directa
}

size_t Interconnect::getConnectedPECount() const {
    return caches.size();
}

void Interconnect::clearMessages() {
    while (!message_queue.empty()) {
        message_queue.pop();
    }
}