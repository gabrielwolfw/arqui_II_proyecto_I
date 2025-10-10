#include "interconnect.hpp"

Interconnect::Interconnect(unsigned int num_processing_elements, bool verbose_mode) 
    : current_pe(0), num_pes(num_processing_elements), verbose(verbose_mode) {}

void Interconnect::debugPrint(const std::string& message) const {
    if (verbose) {
        std::cout << "[Interconnect] " << message << std::endl;
    }
}

void Interconnect::addRequest(const BusTransaction& trans) {
    pending_requests.addTransaction(trans);
    debugPrint("New request added from PE " + std::to_string(trans.pe_id));
}

void Interconnect::arbitrate() {
    auto& pending = pending_requests.getTransactions();
    bool request_processed = false;
    
    // Buscar la siguiente transacción del PE actual
    for (auto& trans : pending) {
        if (trans.pe_id == current_pe && !trans.processed) {
            // Procesar la transacción
            trans.processed = true;
            processed_requests.addTransaction(trans);
            
            debugPrint("Processing transaction from PE " + std::to_string(trans.pe_id) + 
                      " - Operation: " + std::to_string(static_cast<int>(trans.op_type)));
            
            request_processed = true;
            break;
        }
    }
    
    // Actualizar el PE actual (round-robin)
    current_pe = (current_pe + 1) % num_pes;
    
    if (!request_processed) {
        debugPrint("No pending requests for PE " + std::to_string(current_pe));
    }
}