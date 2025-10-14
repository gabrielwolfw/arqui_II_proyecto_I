#ifndef BUS_INTERFACE_H
#define BUS_INTERFACE_H

#include <cstdint>
#include <cstddef>  // Para size_t

// Forward declarations
enum class BusEvent;
struct BusMessage;

// Interfaz abstracta para comunicación con el bus
class IBusInterface {
public:
    virtual ~IBusInterface() = default;
    
    // Enviar mensaje al bus
    virtual void sendMessage(const BusMessage& msg) = 0;
    
    // Leer desde memoria
    virtual void readFromMemory(uint64_t address, uint8_t* data, size_t size) = 0;
    
    // Escribir a memoria
    virtual void writeToMemory(uint64_t address, const uint8_t* data, size_t size) = 0;
    
    // Transferencia cache-to-cache (opcional)
    virtual void supplyData(uint64_t address, const uint8_t* data) = 0;
};

#endif // BUS_INTERFACE_H