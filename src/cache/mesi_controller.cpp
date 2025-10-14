#include "mesi_controller.hpp"
#include <iostream>

MESIController::MESIController(int pe_id) : pe_id(pe_id), transition_count(0) {}

MESIResult MESIController::processEvent(MESIState current_state, BusEvent event) {
    MESIResult result;
    
    switch (current_state) {
        case MESIState::INVALID:
            result = handleInvalidState(event);
            break;
        case MESIState::SHARED:
            result = handleSharedState(event);
            break;
        case MESIState::EXCLUSIVE:
            result = handleExclusiveState(event);
            break;
        case MESIState::MODIFIED:
            result = handleModifiedState(event);
            break;
    }
    
    if (result.new_state != current_state) {
        transition_count++;
        std::cout << "[PE" << pe_id << "] MESI: " 
                  << getStateName(current_state) << " -> " 
                  << getStateName(result.new_state) 
                  << " (Event: " << getEventName(event) << ")" << std::endl;
    }
    
    return result;
}

MESIResult MESIController::handleInvalidState(BusEvent event) {
    MESIResult result;
    result.new_state = MESIState::INVALID;
    
    switch (event) {
        case BusEvent::LOCAL_READ:
            // I -> E o I -> S (depende de respuesta de otros PEs)
            // Enviamos BUS_READ y esperamos respuesta
            result.new_state = MESIState::EXCLUSIVE;  // Asumimos E, se ajustará a S si otro responde
            result.needs_bus_message = true;          // Enviar BUS_READ por interconnect
            result.fetch_from_memory = true;          // Siempre va a memoria (puede ser interceptado)
            break;
            
        case BusEvent::LOCAL_WRITE:
            // I -> M
            result.new_state = MESIState::MODIFIED;
            result.needs_bus_message = true;          // Enviar BUS_READX para invalidar otros
            result.fetch_from_memory = true;          // Va a memoria por el bloque
            break;
            
        case BusEvent::BUS_READ:
        case BusEvent::BUS_READX:
        case BusEvent::BUS_UPGRADE:
            // Permanece en I, no hace nada (no tiene el dato)
            result.new_state = MESIState::INVALID;
            break;
            
        case BusEvent::EVICTION:
            // Ya está inválido
            result.new_state = MESIState::INVALID;
            break;
    }
    
    return result;
}

MESIResult MESIController::handleSharedState(BusEvent event) {
    MESIResult result;
    result.new_state = MESIState::SHARED;
    
    switch (event) {
        case BusEvent::LOCAL_READ:
            // S -> S (hit, no cambia)
            result.new_state = MESIState::SHARED;
            break;
            
        case BusEvent::LOCAL_WRITE:
            // S -> M (debe invalidar otras copias)
            result.new_state = MESIState::MODIFIED;
            result.needs_bus_message = true;  // Enviar BUS_UPGRADE
            break;
            
        case BusEvent::BUS_READ:
            // S -> S (otro también lee, permanecemos en S)
            result.new_state = MESIState::SHARED;
            result.supply_data = true;  // Podemos proveer el dato si se requiere
            break;
            
        case BusEvent::BUS_READX:
            // S -> I (otro quiere escribir, nos invalidan)
            result.new_state = MESIState::INVALID;
            result.needs_invalidate = true;
            break;
            
        case BusEvent::BUS_UPGRADE:
            // S -> I (otro hace upgrade de S a M)
            result.new_state = MESIState::INVALID;
            result.needs_invalidate = true;
            break;
            
        case BusEvent::EVICTION:
            // S -> I (evicción limpia, no necesita writeback)
            result.new_state = MESIState::INVALID;
            break;
    }
    
    return result;
}

MESIResult MESIController::handleExclusiveState(BusEvent event) {
    MESIResult result;
    result.new_state = MESIState::EXCLUSIVE;
    
    switch (event) {
        case BusEvent::LOCAL_READ:
            // E -> E (hit limpio)
            result.new_state = MESIState::EXCLUSIVE;
            break;
            
        case BusEvent::LOCAL_WRITE:
            // E -> M (escritura silenciosa, sin bus message)
            result.new_state = MESIState::MODIFIED;
            break;
            
        case BusEvent::BUS_READ:
            // E -> S (otro quiere leer, le pasamos el dato directamente)
            result.new_state = MESIState::SHARED;
            result.supply_data = true;  // Le pasamos el dato por interconnect
            break;
            
        case BusEvent::BUS_READX:
            // E -> I (otro quiere escribir)
            result.new_state = MESIState::INVALID;
            result.needs_invalidate = true;
            result.supply_data = true;  // Le pasamos el dato antes de invalidar
            break;
            
        case BusEvent::BUS_UPGRADE:
            // No aplica desde E
            result.new_state = MESIState::EXCLUSIVE;
            break;
            
        case BusEvent::EVICTION:
            // E -> I (evicción limpia, no hay escrituras)
            result.new_state = MESIState::INVALID;
            break;
    }
    
    return result;
}

MESIResult MESIController::handleModifiedState(BusEvent event) {
    MESIResult result;
    result.new_state = MESIState::MODIFIED;
    
    switch (event) {
        case BusEvent::LOCAL_READ:
            // M -> M (hit sucio)
            result.new_state = MESIState::MODIFIED;
            break;
            
        case BusEvent::LOCAL_WRITE:
            // M -> M (hit sucio)
            result.new_state = MESIState::MODIFIED;
            break;
            
        case BusEvent::BUS_READ:
            // M -> S (otro quiere leer)
            // Debemos hacer writeback primero para actualizar memoria
            // Luego el PE solicitante va a memoria
            result.new_state = MESIState::SHARED;
            result.needs_writeback = true;  // Actualizar memoria
            result.supply_data = false;      // NO le pasamos directamente, va a memoria
            break;
            
        case BusEvent::BUS_READX:
            // M -> I (otro quiere escribir)
            // Hacemos writeback primero, luego invalidamos
            result.new_state = MESIState::INVALID;
            result.needs_writeback = true;   // Actualizar memoria primero
            result.needs_invalidate = true;
            result.supply_data = false;      // El otro PE va a memoria después del writeback
            break;
            
        case BusEvent::BUS_UPGRADE:
            // No aplica desde M (nadie más puede tener copia)
            result.new_state = MESIState::MODIFIED;
            break;
            
        case BusEvent::EVICTION:
            // M -> I (evicción sucia, REQUIERE writeback)
            result.new_state = MESIState::INVALID;
            result.needs_writeback = true;
            break;
    }
    
    return result;
}

std::string MESIController::getStateName(MESIState state) {
    switch (state) {
        case MESIState::INVALID:   return "I";
        case MESIState::SHARED:    return "S";
        case MESIState::EXCLUSIVE: return "E";
        case MESIState::MODIFIED:  return "M";
        default: return "?";
    }
}

std::string MESIController::getEventName(BusEvent event) {
    switch (event) {
        case BusEvent::LOCAL_READ:   return "LocalRead";
        case BusEvent::LOCAL_WRITE:  return "LocalWrite";
        case BusEvent::BUS_READ:     return "BusRead";
        case BusEvent::BUS_READX:    return "BusReadX";
        case BusEvent::BUS_UPGRADE:  return "BusUpgrade";
        case BusEvent::EVICTION:     return "Eviction";
        default: return "?";
    }
}