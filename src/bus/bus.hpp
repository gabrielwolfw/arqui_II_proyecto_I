#ifndef BUS_HPP
#define BUS_HPP

#include <string>
#include <vector>

// Tipos de transacciones soportadas por el bus
enum class BusOpType {
    BusRd,   // Lectura compartida de un bloque
    BusRdX,  // Lectura con intención de modificar (lectura exclusiva)
    BusUpgr, // Solicitud para pasar de S → M sin recargar el bloque
    BusWB    // Escritura a memoria (cuando un bloque M se reemplaza)
};

// Estructura para representar una transacción en el bus
struct BusTransaction {
    unsigned int address;     // Dirección del bloque
    unsigned int pe_id;      // Identificador del PE emisor
    BusOpType op_type;       // Tipo de operación
    bool processed;          // Flag para indicar si la transacción fue procesada
    
    BusTransaction(unsigned int addr, unsigned int id, BusOpType op) :
        address(addr), pe_id(id), op_type(op), processed(false) {}
};

// Lista de transacciones del bus
class BusList {
private:
    std::vector<BusTransaction> transactions;

public:
    void addTransaction(const BusTransaction& trans);
    std::vector<BusTransaction>& getTransactions() { return transactions; }
    const std::vector<BusTransaction>& getTransactions() const { return transactions; }
};

#endif // BUS_HPP