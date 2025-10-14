#ifndef LRU_POLICY_H
#define LRU_POLICY_H

#include <cstdint>
#include <array>
#include <cstddef>

constexpr size_t MAX_WAYS = 4;  // Soporta hasta 4-way

class LRUPolicy {
private:
    size_t num_ways;
    // Matriz de bits LRU: lru_matrix[i][j] = 1 significa que way i fue usado más recientemente que way j
    std::array<std::array<bool, MAX_WAYS>, MAX_WAYS> lru_matrix;
    
public:
    LRUPolicy(size_t ways);
    
    // Actualiza la política LRU cuando se accede a un way
    void access(int way_accessed);
    
    // Encuentra el way menos recientemente usado
    int findVictim() const;
    
    // Resetea la política
    void reset();
    
    // Para debugging
    void print() const;
};

#endif // LRU_POLICY_H