#include "write_policy.h"

WritePolicy::WritePolicy(WriteHitPolicy hit, WriteMissPolicy miss)
    : hit_policy(hit), miss_policy(miss) {}

bool WritePolicy::handleWriteHit() const {
    // Write-back: marcar como dirty, no escribir a memoria todavía
    // Write-through: escribir a memoria inmediatamente
    return (hit_policy == WriteHitPolicy::WRITE_BACK);
}

bool WritePolicy::handleWriteMiss() const {
    // Write-allocate: traer bloque a caché
    // No-write-allocate: escribir directo a memoria sin traer bloque
    return (miss_policy == WriteMissPolicy::WRITE_ALLOCATE);
}