#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include "bus.hpp"
#include <queue>
#include <map>
#include <iostream>

class Interconnect {
private:
    BusList pending_requests;     // Lista de requests pendientes
    BusList processed_requests;   // Lista de requests procesados
    unsigned int current_pe;      // PE actual para round-robin
    unsigned int num_pes;         // Número total de PEs
    bool verbose;                 // Flag para modo verbose

    // Método privado para imprimir mensajes en modo verbose
    void debugPrint(const std::string& message) const;

public:
    Interconnect(unsigned int num_processing_elements, bool verbose_mode = false);
    
    // Añade un nuevo request al bus
    void addRequest(const BusTransaction& trans);
    
    // Procesa un ciclo de arbitración
    void arbitrate();
    
    // Getters
    const BusList& getPendingRequests() const { return pending_requests; }
    const BusList& getProcessedRequests() const { return processed_requests; }
};

#endif // INTERCONNECT_HPP