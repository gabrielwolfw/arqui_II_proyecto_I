#include "cache.h"
#include <iostream>
#include <iomanip>
#include <cstring>

Cache::Cache(int pe_id) : pe_id(pe_id), interconnect(nullptr) {
    // Inicialización ya se hace en los constructores por defecto
}

// Encuentra si el bloque está en caché y en qué way
int Cache::findWay(uint8_t index, uint64_t tag) {
    for (int way = 0; way < CACHE_WAYS; way++) {
        CacheLine& line = cache_sets[index].ways[way];
        if (line.valid && line.tag == tag) {
            return way;  // Hit
        }
    }
    return -1;  // Miss
}

// Selecciona la víctima para reemplazo usando LRU
int Cache::selectVictim(uint8_t index) {
    // Primero busca si hay alguna línea inválida
    for (int way = 0; way < CACHE_WAYS; way++) {
        if (!cache_sets[index].ways[way].valid) {
            return way;
        }
    }
    
    // Si ambas son válidas, usa LRU
    // LRU bit = 0 significa menos recientemente usado
    if (cache_sets[index].ways[0].lru_bit == false) {
        return 0;
    } else {
        return 1;
    }
}

// Actualiza los bits LRU después de un acceso
void Cache::updateLRU(uint8_t index, int way_accessed) {
    // En 2-way: el way accedido se marca como 1 (más reciente)
    // El otro se marca como 0 (menos reciente)
    cache_sets[index].ways[way_accessed].lru_bit = true;
    cache_sets[index].ways[1 - way_accessed].lru_bit = false;
}

// Write-back: escribe una línea modificada a memoria
void Cache::writebackLine(uint8_t index, int way) {
    CacheLine& line = cache_sets[index].ways[way];
    
    if (line.dirty && line.valid) {
        // Reconstruir la dirección
        uint64_t address = (line.tag << (OFFSET_BITS + INDEX_BITS)) | 
                          (index << OFFSET_BITS);
        
        // TODO: Escribir a memoria principal a través del interconnect
        // interconnect->writeToMemory(address, line.data);
        
        stats.writebacks++;
        line.dirty = false;
        
        std::cout << "[PE" << pe_id << "] Writeback: addr=0x" 
                  << std::hex << address << std::dec << std::endl;
    }
}

// Operación de lectura
bool Cache::read(uint64_t address, uint64_t& data) {
    Address addr(address);
    int way = findWay(addr.index, addr.tag);
    
    if (way != -1) {
        // Cache HIT
        stats.read_hits++;
        CacheLine& line = cache_sets[addr.index].ways[way];
        
        // Extraer el dato (asumiendo que es un uint64_t)
        std::memcpy(&data, &line.data[addr.offset], sizeof(uint64_t));
        
        updateLRU(addr.index, way);
        
        std::cout << "[PE" << pe_id << "] READ HIT: addr=0x" 
                  << std::hex << address << " data=0x" << data << std::dec << std::endl;
        
        return true;
    } else {
        // Cache MISS
        stats.read_misses++;
        
        std::cout << "[PE" << pe_id << "] READ MISS: addr=0x" 
                  << std::hex << address << std::dec << std::endl;
        
        // Seleccionar víctima
        int victim_way = selectVictim(addr.index);
        
        // Si la víctima está modificada, hacer writeback
        writebackLine(addr.index, victim_way);
        
        // Cargar bloque desde memoria (o de otra caché vía MESI)
        CacheLine& line = cache_sets[addr.index].ways[victim_way];
        
        // TODO: Implementar protocolo MESI para obtener el bloque
        // Por ahora, simulamos carga desde memoria
        // interconnect->requestBlock(address, line.data);
        
        // Actualizar metadatos
        line.valid = true;
        line.tag = addr.tag;
        line.dirty = false;
        line.mesi_state = MESIState::EXCLUSIVE;  // Simplificado, MESI lo ajustará
        
        // Extraer dato
        std::memcpy(&data, &line.data[addr.offset], sizeof(uint64_t));
        
        updateLRU(addr.index, victim_way);
        
        return false;  // Indica que fue miss
    }
}

// Operación de escritura
bool Cache::write(uint64_t address, uint64_t data) {
    Address addr(address);
    int way = findWay(addr.index, addr.tag);
    
    if (way != -1) {
        // Cache HIT
        stats.write_hits++;
        CacheLine& line = cache_sets[addr.index].ways[way];
        
        // Write-back: solo escribimos en caché
        std::memcpy(&line.data[addr.offset], &data, sizeof(uint64_t));
        line.dirty = true;
        
        // Transición MESI a Modified
        if (line.mesi_state != MESIState::MODIFIED) {
            transitionMESI(addr.index, way, MESIState::MODIFIED);
        }
        
        updateLRU(addr.index, way);
        
        std::cout << "[PE" << pe_id << "] WRITE HIT: addr=0x" 
                  << std::hex << address << " data=0x" << data << std::dec << std::endl;
        
        return true;
    } else {
        // Cache MISS - Write-allocate: traer el bloque primero
        stats.write_misses++;
        
        std::cout << "[PE" << pe_id << "] WRITE MISS: addr=0x" 
                  << std::hex << address << std::dec << std::endl;
        
        int victim_way = selectVictim(addr.index);
        writebackLine(addr.index, victim_way);
        
        CacheLine& line = cache_sets[addr.index].ways[victim_way];
        
        // TODO: Cargar bloque desde memoria
        // interconnect->requestBlockForWrite(address, line.data);
        
        line.valid = true;
        line.tag = addr.tag;
        line.dirty = true;
        line.mesi_state = MESIState::MODIFIED;
        
        // Escribir el dato
        std::memcpy(&line.data[addr.offset], &data, sizeof(uint64_t));
        
        updateLRU(addr.index, victim_way);
        
        return false;
    }
}

// Transición de estados MESI
void Cache::transitionMESI(uint8_t index, int way, MESIState new_state) {
    CacheLine& line = cache_sets[index].ways[way];
    MESIState old_state = line.mesi_state;
    
    if (old_state != new_state) {
        line.mesi_state = new_state;
        stats.mesi_transitions++;
        
        std::cout << "[PE" << pe_id << "] MESI Transition: "
                  << static_cast<int>(old_state) << " -> " 
                  << static_cast<int>(new_state) << std::endl;
    }
}

// Imprimir estadísticas
void Cache::printStats() const {
    std::cout << "\n=== Cache Statistics for PE" << pe_id << " ===" << std::endl;
    std::cout << "Read Hits: " << stats.read_hits << std::endl;
    std::cout << "Read Misses: " << stats.read_misses << std::endl;
    std::cout << "Write Hits: " << stats.write_hits << std::endl;
    std::cout << "Write Misses: " << stats.write_misses << std::endl;
    std::cout << "Invalidations: " << stats.invalidations << std::endl;
    std::cout << "Writebacks: " << stats.writebacks << std::endl;
    std::cout << "MESI Transitions: " << stats.mesi_transitions << std::endl;
    
    uint64_t total_accesses = stats.read_hits + stats.read_misses + 
                               stats.write_hits + stats.write_misses;
    if (total_accesses > 0) {
        double hit_rate = static_cast<double>(stats.read_hits + stats.write_hits) / 
                         total_accesses * 100.0;
        std::cout << "Hit Rate: " << std::fixed << std::setprecision(2) 
                  << hit_rate << "%" << std::endl;
    }
}

// Imprimir contenido de la caché
void Cache::printCache() const {
    std::cout << "\n=== Cache Contents for PE" << pe_id << " ===" << std::endl;
    for (int set = 0; set < CACHE_SETS; set++) {
        std::cout << "Set " << set << ":" << std::endl;
        for (int way = 0; way < CACHE_WAYS; way++) {
            const CacheLine& line = cache_sets[set].ways[way];
            std::cout << "  Way " << way << ": ";
            if (line.valid) {
                std::cout << "V=" << line.valid 
                         << " D=" << line.dirty
                         << " MESI=" << static_cast<int>(line.mesi_state)
                         << " Tag=0x" << std::hex << line.tag << std::dec
                         << " LRU=" << line.lru_bit;
            } else {
                std::cout << "INVALID";
            }
            std::cout << std::endl;
        }
    }
}