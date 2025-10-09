#ifndef MESI_CONTROLLER_H
#define MESI_CONTROLLER_H

#include <cstdint>
#include <string>

// Estados MESI
enum class MESIState : uint8_t {
    INVALID = 0,    // I
    SHARED = 1,     // S
    EXCLUSIVE = 2,  // E
    MODIFIED = 3    // M
};

// Eventos del bus
enum class BusEvent {
    LOCAL_READ,      // Lectura local
    LOCAL_WRITE,     // Escritura local
    BUS_READ,        // Otro PE quiere leer
    BUS_READX,       // Otro PE quiere escribir
    BUS_UPGRADE,     // Upgrade de S a M
    EVICTION         // Desalojo de línea
};

// Resultado de una transición MESI
struct MESIResult {
    MESIState new_state;
    bool needs_bus_transaction;  // ¿Necesita transacción en el bus?
    bool needs_writeback;        // ¿Necesita writeback?
    bool needs_invalidate;       // ¿Necesita invalidar?
    bool needs_data_from_memory; // ¿Necesita traer dato de memoria?
    bool needs_data_from_cache;  // ¿Necesita dato de otra caché?
    
    MESIResult() : new_state(MESIState::INVALID), 
                   needs_bus_transaction(false),
                   needs_writeback(false),
                   needs_invalidate(false),
                   needs_data_from_memory(false),
                   needs_data_from_cache(false) {}
};

class MESIController {
private:
    int pe_id;
    uint64_t transition_count;
    
    // Tabla de transiciones MESI
    MESIResult handleInvalidState(BusEvent event);
    MESIResult handleSharedState(BusEvent event);
    MESIResult handleExclusiveState(BusEvent event);
    MESIResult handleModifiedState(BusEvent event);
    
public:
    MESIController(int pe_id);
    
    // Procesar evento y retornar acciones necesarias
    MESIResult processEvent(MESIState current_state, BusEvent event);
    
    // Obtener nombre del estado
    static std::string getStateName(MESIState state);
    static std::string getEventName(BusEvent event);
    
    // Estadísticas
    uint64_t getTransitionCount() const { return transition_count; }
    void resetStats() { transition_count = 0; }
};

#endif // MESI_CONTROLLER_H