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
            // I -> E o I -> S dependiendo si otros tienen copia
            result.new_state = MESIState::EXCLUSIVE;  // Asumimos E, bus lo ajustará a S si necesario
            result.needs_bus_transaction = true;
            result.needs_data_from_memory = true;
            break;
            
        case BusEvent::LOCAL_WRITE:
            // I -> M
            result.new_state = MESIState::MODIFIED;
            result.needs_bus_transaction = true;
            result.needs_data_from_memory = true;
            break;
            
        case BusEvent::BUS_READ:
        case BusEvent::BUS_READX:
        case BusEvent::BUS_UPGRADE:
            // Permanece en I, no hace nada
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
            result.needs_bus_transaction = true;  // BusUpgrade
            break;
            
        case BusEvent::BUS_READ:
            // S -> S (otro también lee)
            result.new_state = MESIState::SHARED;
            result.needs_data_from_cache = true;  // Puede proveer dato
            break;
            
        case BusEvent::BUS_READX:
            // S -> I (otro quiere escribir)
            result.new_state = MESIState::INVALID;
            result.needs_invalidate = true;
            break;
            
        case BusEvent::BUS_UPGRADE:
            // S -> I (otro hace upgrade)
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
            // E -> M (escritura silenciosa, sin bus transaction)
            result.new_state = MESIState::MODIFIED;
            break;
            
        case BusEvent::BUS_READ:
            // E -> S (otro quiere leer, compartimos)
            result.new_state = MESIState::SHARED;
            result.needs_data_from_cache = true;
            break;
            
        case BusEvent::BUS_READX:
            // E -> I (otro quiere escribir)
            result.new_state = MESIState::INVALID;
            result.needs_invalidate = true;
            result.needs_data_from_cache = true;
            break;
            
        case BusEvent::BUS_UPGRADE:
            // No aplica desde E
            result.new_state = MESIState::EXCLUSIVE;
            break;
            
        case BusEvent::EVICTION:
            // E -> I (evicción limpia)
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
            // M -> S (otro quiere leer, hacemos writeback y compartimos)
            result.new_state = MESIState::SHARED;
            result.needs_writeback = true;
            result.needs_data_from_cache = true;
            break;
            
        case BusEvent::BUS_READX:
            // M -> I (otro quiere escribir, hacemos writeback)
            result.new_state = MESIState::INVALID;
            result.needs_writeback = true;
            result.needs_invalidate = true;
            result.needs_data_from_cache = true;
            break;
            
        case BusEvent::BUS_UPGRADE:
            // No aplica desde M
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