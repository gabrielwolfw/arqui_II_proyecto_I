#ifndef CACHE_H
#define CACHE_H

#include <cstdint>
#include <array>
#include <memory>
#include <vector>

// Constantes de configuración
constexpr size_t CACHE_BLOCK_SIZE = 32;      // 32 bytes por bloque
constexpr size_t CACHE_WAYS = 2;             // 2-way set associative
constexpr size_t CACHE_SETS = 8;             // 8 conjuntos
constexpr size_t CACHE_BLOCKS = 16;          // 16 bloques totales
constexpr size_t CACHE_SIZE = 512;           // 512 bytes total

constexpr size_t OFFSET_BITS = 5;            // log2(32)
constexpr size_t INDEX_BITS = 3;             // log2(8)
constexpr size_t TAG_BITS = 52;              // Resto para el tag

// Estados MESI
enum class MESIState : uint8_t {
    INVALID = 0b00,
    SHARED = 0b01,
    EXCLUSIVE = 0b10,
    MODIFIED = 0b11
};

// Estructura de una línea de caché
struct CacheLine {
    bool valid;                                      // 1 bit
    bool dirty;                                      // 1 bit
    MESIState mesi_state;                           // 2 bits
    uint64_t tag;                                   // 52 bits (usamos 64 bits por practicidad)
    std::array<uint8_t, CACHE_BLOCK_SIZE> data;    // 32 bytes = 256 bits
    bool lru_bit;                                   // 1 bit para LRU

    // Constructor
    CacheLine() : valid(false), dirty(false), 
                  mesi_state(MESIState::INVALID), 
                  tag(0), lru_bit(false) {
        data.fill(0);
    }
};

// Estructura de un conjunto (set)
struct CacheSet {
    std::array<CacheLine, CACHE_WAYS> ways;  // 2 ways por set
    
    CacheSet() = default;
};

// Estructura de dirección descompuesta
struct Address {
    uint64_t tag;
    uint8_t index;
    uint8_t offset;
    
    // Constructor que descompone una dirección
    Address(uint64_t addr) {
        offset = addr & 0x1F;                    // 5 bits menos significativos
        index = (addr >> OFFSET_BITS) & 0x7;     // Siguientes 3 bits
        tag = addr >> (OFFSET_BITS + INDEX_BITS); // Resto para tag
    }
};

// Estadísticas de la caché
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

// Clase principal de la caché
class Cache {
private:
    int pe_id;                                       // ID del Processing Element
    std::array<CacheSet, CACHE_SETS> cache_sets;    // 8 sets
    CacheStats stats;                                // Estadísticas
    
    // Forward declaration para el Interconnect (lo implementarás después)
    class Interconnect* interconnect;
    
    // Métodos privados auxiliares
    int findWay(uint8_t index, uint64_t tag);
    int selectVictim(uint8_t index);
    void updateLRU(uint8_t index, int way);
    void writebackLine(uint8_t index, int way);
    
public:
    // Constructor
    Cache(int pe_id);
    
    // Operaciones principales
    bool read(uint64_t address, uint64_t& data);
    bool write(uint64_t address, uint64_t data);
    
    // Métodos para protocolo MESI
    void handleBusRead(uint64_t address);
    void handleBusReadX(uint64_t address);
    void handleBusUpgrade(uint64_t address);
    void invalidateLine(uint64_t address);
    
    // Gestión de estado MESI
    void transitionMESI(uint8_t index, int way, MESIState new_state);
    MESIState getMESIState(uint64_t address);
    
    // Utilidades
    void setInterconnect(Interconnect* ic) { interconnect = ic; }
    CacheStats getStats() const { return stats; }
    void printCache() const;
    void printStats() const;
    
    // Debugging
    CacheLine* getCacheLine(uint64_t address);
};

#endif // CACHE_H