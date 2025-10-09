#include "lru_policy.hpp"
#include <iostream>
#include <iomanip>

LRUPolicy::LRUPolicy(size_t ways) : num_ways(ways) {
    reset();
}

void LRUPolicy::reset() {
    for (size_t i = 0; i < MAX_WAYS; i++) {
        for (size_t j = 0; j < MAX_WAYS; j++) {
            lru_matrix[i][j] = false;
        }
    }
}

void LRUPolicy::access(int way_accessed) {
    // Cuando accedemos al way_accessed:
    // - Ponemos 1 en lru_matrix[way_accessed][*] (más reciente que todos)
    // - Ponemos 0 en lru_matrix[*][way_accessed] (todos son menos recientes que este)
    
    for (size_t i = 0; i < num_ways; i++) {
        if (i != static_cast<size_t>(way_accessed)) {
            lru_matrix[way_accessed][i] = true;   // way_accessed es más reciente que i
            lru_matrix[i][way_accessed] = false;  // i es menos reciente que way_accessed
        }
    }
}

int LRUPolicy::findVictim() const {
    // El way con todos 0s en su fila es el menos recientemente usado
    for (size_t i = 0; i < num_ways; i++) {
        bool is_lru = true;
        for (size_t j = 0; j < num_ways; j++) {
            if (i != j && lru_matrix[i][j]) {
                is_lru = false;
                break;
            }
        }
        if (is_lru) {
            return static_cast<int>(i);
        }
    }
    
    // Fallback: retornar way 0
    return 0;
}

void LRUPolicy::print() const {
    std::cout << "LRU Matrix:" << std::endl;
    std::cout << "    ";
    for (size_t j = 0; j < num_ways; j++) {
        std::cout << "W" << j << " ";
    }
    std::cout << std::endl;
    
    for (size_t i = 0; i < num_ways; i++) {
        std::cout << "W" << i << ": ";
        for (size_t j = 0; j < num_ways; j++) {
            std::cout << (lru_matrix[i][j] ? " 1 " : " 0 ");
        }
        std::cout << std::endl;
    }
}