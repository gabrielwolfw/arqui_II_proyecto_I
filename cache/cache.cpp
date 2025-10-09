#include "cache.h"
#include <iostream>
#include <iomanip>
#include <cstring>

Cache::Cache(int pe_id) : pe_id(pe_id), interconnect(nullptr) {
    // Inicializar componentes modulares
    mesi_controller = std::make_unique<MESIController>(pe_id);
    write_policy = std::make_unique<WritePolicy>(
        WriteHitPolicy::WRITE_BACK,
        WriteMissPolicy::WRITE_ALLOCATE
    );
}

int Cache::findWay(uint8_t index, uint64_t tag) {
    for (int way = 0; way < CACHE_WAYS; way++) {
        CacheLine& line = cache_sets[index].ways[way];
        if (line.valid && line.tag == tag) {
            return way;
        }
    }
    return -1;
}

int Cache::selectVictim(uint8_t index) {
    // Primero buscar líneas inválidas
    for (int way = 0; way < CACHE_WAYS; way++) {
        if (!cache_sets[index].ways[way].valid) {
            return way;
        }
    }
    
    // Si todas son válidas, usar LRU
    return cache_sets[index].lru->findVictim();
}

void Cache::writebackLine(uint8_t index, int way) {
    CacheLine& line = cache_sets[index].ways[way];
    
    if (line.dirty && line.valid) {
        uint64_t address = (line.tag << (OFFSET_BITS + INDEX_BITS)) | 
                          (index << OFFSET_BITS);
        
        // TODO: Escribir a memoria a través del interconnect
        // interconnect->writeToMemory(address, line.data.data(), CACHE_BLOCK_SIZE);
        
        stats.writebacks++;
        line.dirty = false;
        
        std::cout << "[PE" << pe_id << "] Writeback: addr=0x" 
                  << std::hex << address << std::dec << std::endl;
    }
}

bool Cache::fetchBlock(uint64_t address, uint8_t* data) {
    // TODO: Implementar fetch desde memoria o interconnect
    // Por ahora, simula lectura con datos dummy
    std::memset(data, 0xAB, CACHE_BLOCK_SIZE);
    
    std::cout << "[PE" << pe_id << "] Fetching block from memory: addr=0x" 
              << std::hex << address << std::dec << std::endl;
    
    return true;
}

bool Cache::read(uint64_t address, uint64_t& data) {
    Address addr(address);
    int way = findWay(addr.index, addr.tag);
    
    if (way != -1) {
        // **HIT**
        stats.read_hits++;
        CacheLine& line = cache_sets[addr.index].ways[way];
        
        // Procesar con MESI
        MESIResult mesi_result = mesi_controller->processEvent(
            line.mesi_state, BusEvent::LOCAL_READ
        );
        
        line.mesi_state = mesi_result.new_state;
        stats.mesi_transitions += (mesi_result.new_state != line.mesi_state) ? 1 : 0;
        
        // Extraer dato
        std::memcpy(&data, &line.data[addr.offset], sizeof(uint64_t));
        
        // Actualizar LRU
        cache_sets[addr.index].lru->access(way);
        
        std::cout << "[PE" << pe_id << "] READ HIT: addr=0x" 
                  << std::hex << address << " data=0x" << data << std::dec 
                  << " [" << MESIController::getStateName(line.mesi_state) << "]" << std::endl;
        
        return true;
        
    } else {
        // **MISS**
        stats.read_misses++;
        
        std::cout << "[PE" << pe_id << "] READ MISS: addr=0x" 
                  << std::hex << address << std::dec << std::endl;
        
        // Seleccionar víctima con LRU
        int victim_way = selectVictim(addr.index);
        CacheLine& line = cache_sets[addr.index].ways[victim_way];
        
        // Procesar evicción con MESI si la línea es válida
        if (line.valid) {
            MESIResult evict_result = mesi_controller->processEvent(
                line.mesi_state, BusEvent::EVICTION
            );
            
            if (evict_result.needs_writeback) {
                writebackLine(addr.index, victim_way);
            }
        }
        
        // Traer bloque desde memoria
        fetchBlock(address, line.data.data());
        
        // Procesar con MESI (transición desde INVALID)
        MESIResult mesi_result = mesi_controller->processEvent(
            MESIState::INVALID, BusEvent::LOCAL_READ
        );
        
        // Actualizar metadatos
        line.valid = true;
        line.tag = addr.tag;
        line.dirty = false;
        line.mesi_state = mesi_result.new_state;
        
        // Extraer dato
        std::memcpy(&data, &line.data[addr.offset], sizeof(uint64_t));
        
        // Actualizar LRU
        cache_sets[addr.index].lru->access(victim_way);
        
        stats.mesi_transitions++;
        
        return false;
    }
}

bool Cache::write(uint64_t address, uint64_t data) {
    Address addr(address);
    int way = findWay(addr.index, addr.tag);
    
    if (way != -1) {
        // **HIT**
        stats.write_hits++;
        CacheLine& line = cache_sets[addr.index].ways[way];
        
        // Procesar con MESI
        MESIResult mesi_result = mesi_controller->processEvent(
            line.mesi_state, BusEvent::LOCAL_WRITE
        );
        
        line.mesi_state = mesi_result.new_state;
        
        // Escribir dato en caché
        std::memcpy(&line.data[addr.offset], &data, sizeof(uint64_t));
        
        // Aplicar política de escritura (Write-Back)
        if (write_policy->handleWriteHit()) {
            line.dirty = true;  // Write-back: solo marcar dirty
        }
        
        // Actualizar LRU
        cache_sets[addr.index].lru->access(way);
        
        if (mesi_result.new_state != line.mesi_state) {
            stats.mesi_transitions++;
        }
        
        std::cout << "[PE" << pe_id << "] WRITE HIT: addr=0x" 
                  << std::hex << address << " data=0x" << data << std::dec 
                  << " [" << MESIController::getStateName(line.mesi_state) << "]" << std::endl;
        
        return true;
        
    } else {
        // **MISS**
        stats.write_misses++;
        
        std::cout << "[PE" << pe_id << "] WRITE MISS: addr=0x" 
                  << std::hex << address << std::dec << std::endl;
        
        // Verificar política de Write-Allocate
        if (!write_policy->handleWriteMiss()) {
            // No-write-allocate: escribir directo a memoria
            std::cout << "[PE" << pe_id << "] No-write-allocate: writing to memory" << std::endl;
            return false;
        }
        
        // Write-Allocate: traer el bloque primero
        int victim_way = selectVictim(addr.index);
        CacheLine& line = cache_sets[addr.index].ways[victim_way];
        
        // Procesar evicción
        if (line.valid) {
            MESIResult evict_result = mesi_controller->processEvent(
                line.mesi_state, BusEvent::EVICTION
            );
            
            if (evict_result.needs_writeback) {
                writebackLine(addr.index, victim_way);
            }
        }
        
        // Traer bloque desde memoria
        fetchBlock(address, line.data.data());
        
        // Procesar con MESI
        MESIResult mesi_result = mesi_controller->processEvent(
            MESIState::INVALID, BusEvent::LOCAL_WRITE
        );
        
        // Actualizar metadatos
        line.valid = true;
        line.tag = addr.tag;
        line.dirty = true;  // Write-back con write-allocate
        line.mesi_state = mesi_result.new_state;
        
        // Escribir el dato
        std::memcpy(&line.data[addr.offset], &data, sizeof(uint64_t));
        
        // Actualizar LRU
        cache_sets[addr.index].lru->access(victim_way);
        
        stats.mesi_transitions++;
        
        return false;
    }
}

void Cache::handleBusRead(uint64_t address) {
    Address addr(address);
    int way = findWay(addr.index, addr.tag);
    
    if (way != -1) {
        CacheLine& line = cache_sets[addr.index].ways[way];
        
        MESIResult result = mesi_controller->processEvent(
            line.mesi_state, BusEvent::BUS_READ
        );
        
        if (result.needs_writeback) {
            writebackLine(addr.index, way);
        }
        
        line.mesi_state = result.new_state;
        
        if (result.needs_data_from_cache) {
            std::cout << "[PE" << pe_id << "] Supplying data for BusRead: addr=0x" 
                      << std::hex << address << std::dec << std::endl;
        }
        
        stats.mesi_transitions++;
    }
}

void Cache::handleBusReadX(uint64_t address) {
    Address addr(address);
    int way = findWay(addr.index, addr.tag);
    
    if (way != -1) {
        CacheLine& line = cache_sets[addr.index].ways[way];
        
        MESIResult result = mesi_controller->processEvent(
            line.mesi_state, BusEvent::BUS_READX
        );
        
        if (result.needs_writeback) {
            writebackLine(addr.index, way);
        }
        
        if (result.needs_invalidate) {
            line.valid = false;
            line.mesi_state = MESIState::INVALID;
            stats.invalidations++;
        }
        
        stats.mesi_transitions++;
        
        std::cout << "[PE" << pe_id << "] Invalidated line due to BusReadX: addr=0x" 
                  << std::hex << address << std::dec << std::endl;
    }
}

void Cache::invalidateLine(uint64_t address) {
    Address addr(address);
    int way = findWay(addr.index, addr.tag);
    
    if (way != -1) {
        CacheLine& line = cache_sets[addr.index].ways[way];
        line.valid = false;
        line.mesi_state = MESIState::INVALID;
        stats.invalidations++;
        
        std::cout << "[PE" << pe_id << "] Line invalidated: addr=0x" 
                  << std::hex << address << std::dec << std::endl;
    }
}

void Cache::printStats() const {
    std::cout << "\n=== Cache Statistics for PE" << pe_id << " ===" << std::endl;
    std::cout << "Read Hits: " << stats.read_hits << std::endl;
    std::cout << "Read Misses: " << stats.read_misses << std::endl;
    std::cout << "Write Hits: " << stats.write_hits << std::endl;
    std::cout << "Write Misses: " << stats.write_misses << std::endl;
    std::cout << "Invalidations: " << stats.invalidations << std::endl;
    std::cout << "Writebacks: " << stats.writebacks << std::endl;
    std::cout << "MESI Transitions: " << mesi_controller->getTransitionCount() << std::endl;
    
    uint64_t total_accesses = stats.read_hits + stats.read_misses + 
                               stats.write_hits + stats.write_misses;
    if (total_accesses > 0) {
        double hit_rate = static_cast<double>(stats.read_hits + stats.write_hits) / 
                         total_accesses * 100.0;
        std::cout << "Hit Rate: " << std::fixed << std::setprecision(2) 
                  << hit_rate << "%" << std::endl;
    }
}

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
                         << " MESI=" << MESIController::getStateName(line.mesi_state)
                         << " Tag=0x" << std::hex << line.tag << std::dec;
            } else {
                std::cout << "INVALID";
            }
            std::cout << std::endl;
        }
        std::cout << "  LRU victim would be: Way " << cache_sets[set].lru->findVictim() << std::endl;
    }
}

void Cache::printLRUState(uint8_t index) const {
    std::cout << "\n=== LRU State for Set " << static_cast<int>(index) << " ===" << std::endl;
    cache_sets[index].lru->print();
    std::cout << std::endl;
}