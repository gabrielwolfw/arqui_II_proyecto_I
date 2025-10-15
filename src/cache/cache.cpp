#include "cache.hpp"
#include <iostream>
#include <iomanip>
#include <cstring>

Cache::Cache(int pe_id) : pe_id(pe_id), bus_interface(nullptr) {
    // Inicializar componentes modulares
    mesi_controller = std::make_unique<MESIController>(pe_id);
    write_policy = std::make_unique<WritePolicy>(
        WriteHitPolicy::WRITE_BACK,
        WriteMissPolicy::WRITE_ALLOCATE
    );
}

int Cache::findWay(uint8_t index, uint64_t tag) {
    for (size_t way = 0; way < CACHE_WAYS; way++) {
        CacheLine& line = cache_sets[index].ways[way];
        if (line.valid && line.tag == tag) {
            return static_cast<int>(way);
        }
    }
    return -1;
}

int Cache::selectVictim(uint8_t index) {
    // Primero buscar líneas inválidas
    for (size_t way = 0; way < CACHE_WAYS; way++) {
        if (!cache_sets[index].ways[way].valid) {
            return static_cast<int>(way);
        }
    }
    
    // Si todas son válidas, usar LRU
    return cache_sets[index].lru->findVictim();
}

void Cache::writebackLine(uint8_t index, int way) {
    CacheLine& line = cache_sets[index].ways[way];
    
    if (line.dirty && line.valid) {
        // Reconstruir dirección base del bloque completo
        uint64_t address = (line.tag << (OFFSET_BITS + INDEX_BITS)) | 
                          (index << OFFSET_BITS);
        
        // Escribir a memoria a través del bus interface (si existe)
        if (bus_interface != nullptr) {
            bus_interface->writeToMemory(address, line.data.data(), CACHE_BLOCK_SIZE);
        }
        
        stats.writebacks++;
        line.dirty = false;
        
        std::cout << "[PE" << pe_id << "] Writeback to MEMORY: addr=0x" 
                  << std::hex << address << std::dec;
        if (bus_interface == nullptr) {
            std::cout << " (No bus connected - standalone mode)";
        }
        std::cout << std::endl;
    }
}

bool Cache::fetchBlock(uint64_t address, uint8_t* data) {
    // IMPORTANTE: Alinear la dirección al inicio del bloque de cache
    uint64_t block_address = (address / CACHE_BLOCK_SIZE) * CACHE_BLOCK_SIZE;
    
    // Fetch desde memoria a través del bus interface
    if (bus_interface != nullptr) {
        bus_interface->readFromMemory(block_address, data, CACHE_BLOCK_SIZE);
    } else {
        // Modo standalone: simula lectura con datos dummy
        std::memset(data, 0xAB, CACHE_BLOCK_SIZE);
        std::cout << "[PE" << pe_id << "] Fetching block (standalone mode): addr=0x" 
                  << std::hex << block_address << std::dec << std::endl;
    }
    
    return true;
}

bool Cache::read(uint64_t address, uint64_t& data) {
    Address addr(address);
    int way = findWay(addr.index, addr.tag);
    
    if (way != -1) {
        // **HIT de lectura**
        stats.read_hits++;
        CacheLine& line = cache_sets[addr.index].ways[way];
        
        // Procesar con MESI (evento local)
        MESIResult mesi_result = mesi_controller->processEvent(
            line.mesi_state, BusEvent::LOCAL_READ
        );
        
        line.mesi_state = mesi_result.new_state;
        
        // Extraer dato
        std::memcpy(&data, &line.data[addr.offset], sizeof(uint64_t));
        
        // Actualizar LRU
        cache_sets[addr.index].lru->access(way);
        
        std::cout << "[PE" << pe_id << "] READ HIT: addr=0x" 
                  << std::hex << address << " data=0x" << data << std::dec 
                  << " [" << MESIController::getStateName(line.mesi_state) << "]" << std::endl;
        
        return true;
        
    } else {
        // **MISS de lectura**
        stats.read_misses++;
        
        std::cout << "[PE" << pe_id << "] READ MISS: addr=0x" 
                  << std::hex << address << std::dec << std::endl;
        
        // Seleccionar víctima
        int victim_way = selectVictim(addr.index);
        CacheLine& line = cache_sets[addr.index].ways[victim_way];
        
        // Procesar evicción si la línea es válida
        if (line.valid) {
            MESIResult evict_result = mesi_controller->processEvent(
                line.mesi_state, BusEvent::EVICTION
            );
            
            if (evict_result.needs_writeback) {
                writebackLine(addr.index, victim_way);
            }
        }
        
        // Procesar miss con MESI - Esto genera el mensaje BUS_READ
        MESIResult mesi_result = mesi_controller->processEvent(
            MESIState::INVALID, BusEvent::LOCAL_READ
        );
        
        // Enviar mensaje de BUS_READ al bus (solo si hay bus conectado)
        if (mesi_result.needs_bus_message && bus_interface != nullptr) {
            BusMessage msg{address, BusEvent::BUS_READ, pe_id};
            bus_interface->sendMessage(msg);
            std::cout << "[PE" << pe_id << "] Sending BUS_READ message for addr=0x" 
                      << std::hex << address << std::dec << std::endl;
        }
        
        // SIEMPRE va a memoria
        if (mesi_result.fetch_from_memory) {
            fetchBlock(address, line.data.data());
        }
        
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
        // **HIT de escritura**
        stats.write_hits++;
        CacheLine& line = cache_sets[addr.index].ways[way];
        
        MESIState old_state = line.mesi_state;
        
        // Procesar con MESI (evento local)
        MESIResult mesi_result = mesi_controller->processEvent(
            line.mesi_state, BusEvent::LOCAL_WRITE
        );
        
        // Si estábamos en S y vamos a M, enviamos BUS_UPGRADE (solo si hay bus)
        if (mesi_result.needs_bus_message && bus_interface != nullptr) {
            BusMessage msg{address, BusEvent::BUS_UPGRADE, pe_id};
            bus_interface->sendMessage(msg);
            std::cout << "[PE" << pe_id << "] Sending BUS_UPGRADE message for addr=0x" 
                      << std::hex << address << std::dec << std::endl;
        }
        
        line.mesi_state = mesi_result.new_state;
        
        // Escribir dato en caché
        std::memcpy(&line.data[addr.offset], &data, sizeof(uint64_t));
        
        // Aplicar política de escritura (Write-Back)
        if (write_policy->handleWriteHit()) {
            line.dirty = true;
        }
        
        // Actualizar LRU
        cache_sets[addr.index].lru->access(way);
        
        if (old_state != line.mesi_state) {
            stats.mesi_transitions++;
        }
        
        std::cout << "[PE" << pe_id << "] WRITE HIT: addr=0x" 
                  << std::hex << address << " data=0x" << data << std::dec 
                  << " [" << MESIController::getStateName(line.mesi_state) << "]" << std::endl;
        
        return true;
        
    } else {
        // **MISS de escritura**
        stats.write_misses++;
        
        std::cout << "[PE" << pe_id << "] WRITE MISS: addr=0x" 
                  << std::hex << address << std::dec << std::endl;
        
        // Verificar política de Write-Allocate
        if (!write_policy->handleWriteMiss()) {
            std::cout << "[PE" << pe_id << "] No-write-allocate: writing directly to memory" << std::endl;
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
        
        // Procesar miss con MESI - Genera BUS_READX
        MESIResult mesi_result = mesi_controller->processEvent(
            MESIState::INVALID, BusEvent::LOCAL_WRITE
        );
        
        // Enviar mensaje BUS_READX al bus (solo si hay bus conectado)
        if (mesi_result.needs_bus_message && bus_interface != nullptr) {
            BusMessage msg{address, BusEvent::BUS_READX, pe_id};
            bus_interface->sendMessage(msg);
            std::cout << "[PE" << pe_id << "] Sending BUS_READX message for addr=0x" 
                      << std::hex << address << std::dec << std::endl;
        }
        
        // SIEMPRE va a memoria
        if (mesi_result.fetch_from_memory) {
            fetchBlock(address, line.data.data());
        }
        
        // Actualizar metadatos
        line.valid = true;
        line.tag = addr.tag;
        line.dirty = true;
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
        
        // Si estamos en M, hacemos writeback para que el solicitante vaya a memoria
        if (result.needs_writeback) {
            writebackLine(addr.index, way);
            std::cout << "[PE" << pe_id << "] Writeback due to BUS_READ (M->S), requester will fetch from memory" << std::endl;
        }
        
        // Si estamos en E, proveemos el dato directamente
        if (result.supply_data && bus_interface != nullptr) {
            bus_interface->supplyData(address, line.data.data());
            std::cout << "[PE" << pe_id << "] Supplying data DIRECTLY via interconnect for addr=0x" 
                      << std::hex << address << std::dec << " (E->S)" << std::endl;
        }
        
        line.mesi_state = result.new_state;
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
        
        // Si estamos en M, hacemos writeback primero
        if (result.needs_writeback) {
            writebackLine(addr.index, way);
            std::cout << "[PE" << pe_id << "] Writeback due to BUS_READX (M->I), requester will fetch from memory" << std::endl;
        }
        
        // Si estamos en E, proveemos el dato antes de invalidar
        if (result.supply_data && bus_interface != nullptr) {
            bus_interface->supplyData(address, line.data.data());
            std::cout << "[PE" << pe_id << "] Supplying data DIRECTLY via interconnect for addr=0x" 
                      << std::hex << address << std::dec << " (E->I)" << std::endl;
        }
        
        // Invalidar la línea
        if (result.needs_invalidate) {
            line.valid = false;
            line.mesi_state = MESIState::INVALID;
            stats.invalidations++;
        }
        
        stats.mesi_transitions++;
        
        std::cout << "[PE" << pe_id << "] Invalidated line due to BUS_READX: addr=0x" 
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
    for (size_t set = 0; set < CACHE_SETS; set++) {
        std::cout << "Set " << set << ":" << std::endl;
        for (size_t way = 0; way < CACHE_WAYS; way++) {
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