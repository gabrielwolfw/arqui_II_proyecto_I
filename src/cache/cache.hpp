#ifndef CACHE_UPDATED_H
#define CACHE_UPDATED_H

#include <cstdint>
#include <array>
#include <memory>
#include "lru_policy.hpp"
#include "mesi_controller.hpp"
#include "write_policy.hpp"
#include "bus_interface.hpp"  // Solo la interfaz abstracta

// Constantes de configuración
constexpr size_t CACHE_BLOCK_SIZE = 32;
constexpr size_t CACHE_WAYS = 2;
constexpr size_t CACHE_SETS = 8;
constexpr size_t OFFSET_BITS = 5;
constexpr size_t INDEX_BITS = 3;
constexpr size_t TAG_BITS = 52;

// Mensaje para el bus
struct BusMessage {
    uint64_t address;
    BusEvent event;
    int sender_pe_id;
};

// Estructura de una línea de caché
struct CacheLine {
    bool valid;
    bool dirty;
    MESIState mesi_state;
    uint64_t tag;
    std::array<uint8_t, CACHE_BLOCK_SIZE> data;
    
    CacheLine() : valid(false), dirty(false), 
                  mesi_state(MESIState::INVALID), tag(0) {
        data.fill(0);
    }
};

// Estructura de un conjunto con su propia política LRU
struct CacheSet {
    std::array<CacheLine, CACHE_WAYS> ways;
    std::unique_ptr<LRUPolicy> lru;
    
    CacheSet() : lru(std::make_unique<LRUPolicy>(CACHE_WAYS)) {}
    
    CacheSet(const CacheSet& other) 
        : ways(other.ways),
          lru(std::make_unique<LRUPolicy>(CACHE_WAYS)) {
        *lru = *(other.lru);
    }
    
    CacheSet& operator=(const CacheSet& other) {
        if (this != &other) {
            ways = other.ways;
            lru = std::make_unique<LRUPolicy>(CACHE_WAYS);
            *lru = *(other.lru);
        }
        return *this;
    }
};

struct Address {
    uint64_t tag;
    uint8_t index;
    uint8_t offset;
    
    Address(uint64_t addr) {
        offset = addr & 0x1F;
        index = (addr >> OFFSET_BITS) & 0x7;
        tag = addr >> (OFFSET_BITS + INDEX_BITS);
    }
};

struct CacheStats {
    uint64_t read_hits = 0;
    uint64_t read_misses = 0;
    uint64_t write_hits = 0;
    uint64_t write_misses = 0;
    uint64_t invalidations = 0;
    uint64_t writebacks = 0;
    uint64_t mesi_transitions = 0;
    
    void reset() {
        read_hits = read_misses = write_hits = write_misses = 0;
        invalidations = writebacks = mesi_transitions = 0;
    }
};

class Cache {
private:
    int pe_id;
    std::array<CacheSet, CACHE_SETS> cache_sets;
    CacheStats stats;
    
    // Componentes modulares
    std::unique_ptr<MESIController> mesi_controller;
    std::unique_ptr<WritePolicy> write_policy;
    
    // Interfaz de bus (puede ser nullptr para pruebas standalone)
    IBusInterface* bus_interface;
    
    // Métodos auxiliares
    int findWay(uint8_t index, uint64_t tag);
    int selectVictim(uint8_t index);
    void writebackLine(uint8_t index, int way);
    bool fetchBlock(uint64_t address, uint8_t* data);
    
public:
    Cache(int pe_id);
    
    // Operaciones principales
    bool read(uint64_t address, uint64_t& data);
    bool write(uint64_t address, uint64_t data);
    
    // Protocolo MESI - Reacciones a mensajes del bus
    void handleBusRead(uint64_t address);
    void handleBusReadX(uint64_t address);
    void invalidateLine(uint64_t address);
    
    // Configuración de bus (opcional)
    void setBusInterface(IBusInterface* bus) { bus_interface = bus; }
    
    // Utilidades
    CacheStats getStats() const { return stats; }
    void printCache() const;
    void printStats() const;
    void printLRUState(uint8_t index) const;
};

#endif // CACHE_UPDATED_H