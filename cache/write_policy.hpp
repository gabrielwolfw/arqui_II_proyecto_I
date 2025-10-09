#ifndef WRITE_POLICY_H
#define WRITE_POLICY_H

#include <cstdint>
#include <functional>

// Políticas de escritura
enum class WriteHitPolicy {
    WRITE_BACK,      // Escribe solo en caché
    WRITE_THROUGH    // Escribe en caché y memoria
};

enum class WriteMissPolicy {
    WRITE_ALLOCATE,      // Trae el bloque a caché
    NO_WRITE_ALLOCATE    // Escribe directo a memoria
};

class WritePolicy {
private:
    WriteHitPolicy hit_policy;
    WriteMissPolicy miss_policy;
    
public:
    WritePolicy(WriteHitPolicy hit = WriteHitPolicy::WRITE_BACK,
                WriteMissPolicy miss = WriteMissPolicy::WRITE_ALLOCATE);
    
    // Retorna true si necesita marcar como dirty
    bool handleWriteHit() const;
    
    // Retorna true si necesita traer el bloque
    bool handleWriteMiss() const;
    
    // Getters
    WriteHitPolicy getHitPolicy() const { return hit_policy; }
    WriteMissPolicy getMissPolicy() const { return miss_policy; }
};

#endif // WRITE_POLICY_H