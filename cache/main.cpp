#include "cache.hpp"
#include <iostream>
#include <cassert>

int main() {
    // Crear una caché para PE0
    Cache cache(0);
    
    std::cout << "=== Testing Cache Implementation ===" << std::endl;
    
    // Prueba 1: Lecturas y escrituras básicas
    uint64_t data;
    uint64_t addr1 = 0x0000;  // Set 0
    uint64_t addr2 = 0x0020;  // Set 1
    uint64_t addr3 = 0x0100;  // Set 8 (mismo set que addr1)
    
    std::cout << "\n--- Test 1: Read Miss ---" << std::endl;
    cache.read(addr1, data);
    
    std::cout << "\n--- Test 2: Write Miss (Write-allocate) ---" << std::endl;
    cache.write(addr2, 0xDEADBEEF);
    
    std::cout << "\n--- Test 3: Read Hit ---" << std::endl;
    cache.read(addr1, data);
    
    std::cout << "\n--- Test 4: Write Hit ---" << std::endl;
    cache.write(addr1, 0xCAFEBABE);
    
    std::cout << "\n--- Test 5: LRU Replacement ---" << std::endl;
    cache.read(addr3, data);  // Debería reemplazar addr1
    
    // Imprimir estado
    cache.printCache();
    cache.printStats();

    
    CacheStats stats = cache.getStats();
    std::cout << "Read Hits: " << stats.read_hits << std::endl;
    std::cout << "Read Misses: " << stats.read_misses << std::endl;
    std::cout << "Write Hits: " << stats.write_hits << std::endl;
    std::cout << "Write Misses: " << stats.write_misses << std::endl;

    assert(stats.read_hits == 1);
    assert(stats.read_misses == 2);
    assert(stats.write_hits == 1);
    assert(stats.write_misses == 1);
    std::cout << "All assertions passed!" << std::endl;
    
    return 0;
}