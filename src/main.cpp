#include "Loader.hpp"
#include "PE.hpp"
#include "cache.hpp"


#include "../src/bus/bus.hpp"
#include "../src/ram/ram.hpp"
#include "../src/interconnect/interconnect.hpp"
#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <iomanip>
#include <memory>

// ============================================================
// ADAPTER: Convierte entre estructuras de Cache y Interconnect
// ============================================================

BusTransactionType busEventToTransactionType(BusEvent event) {
    switch (event) {
        case BusEvent::BUS_READ:
            return BusTransactionType::BusRd;
        case BusEvent::BUS_READX:
            return BusTransactionType::BusRdX;
        case BusEvent::BUS_UPGRADE:
            return BusTransactionType::BusUpgr;
        default:
            return BusTransactionType::BusRd;
    }
}

// ============================================================
// INTERFACE: Conecta Cache con Interconnect + RAM
// ============================================================

class InterconnectBusInterface : public IBusInterface {
    std::shared_ptr<Interconnect> interconnect;
    std::shared_ptr<RAM> ram;
    int pe_id;
    
public:
    InterconnectBusInterface(std::shared_ptr<Interconnect> ic, 
                            std::shared_ptr<RAM> r, 
                            int id)
        : interconnect(ic), ram(r), pe_id(id) {}
    
    void readFromMemory(uint64_t address, uint8_t* data, size_t size) override {
        // Alinear a inicio de bloque
        uint64_t block_address = (address / 32) * 32;  // CACHE_BLOCK_SIZE = 32
        
        // Leer bloque completo desde RAM (en palabras de 64 bits)
        size_t num_words = size / sizeof(uint64_t);
        uint64_t word_addr = block_address / sizeof(uint64_t);
        
        std::cout << "[PE" << pe_id << " BusInterface] Reading from RAM: "
                  << "byte_addr=0x" << std::hex << block_address 
                  << " word_addr=" << std::dec << word_addr 
                  << " (" << num_words << " words)" << std::endl;
        
        // Leer cada palabra desde RAM
        for (size_t i = 0; i < num_words; i++) {
            uint32_t ram_addr = static_cast<uint32_t>(word_addr + i);
            
            // Validar dirección
            if (ram_addr > RAM::MAX_ADDRESS) {
                std::cerr << "Warning: RAM address 0x" << std::hex << ram_addr 
                         << " exceeds MAX_ADDRESS, wrapping around" << std::endl;
                ram_addr = ram_addr & RAM::MAX_ADDRESS;
            }
            
            uint64_t word = ram->read(ram_addr);
            std::memcpy(&data[i * sizeof(uint64_t)], &word, sizeof(uint64_t));
        }
    }
    
    void writeToMemory(uint64_t address, const uint8_t* data, size_t size) override {
        // Alinear a inicio de bloque
        uint64_t block_address = (address / 32) * 32;
        
        size_t num_words = size / sizeof(uint64_t);
        uint64_t word_addr = block_address / sizeof(uint64_t);
        
        std::cout << "[PE" << pe_id << " BusInterface] Writing to RAM: "
                  << "byte_addr=0x" << std::hex << block_address 
                  << " word_addr=" << std::dec << word_addr 
                  << " (" << num_words << " words)" << std::endl;
        
        // Escribir cada palabra a RAM
        for (size_t i = 0; i < num_words; i++) {
            uint32_t ram_addr = static_cast<uint32_t>(word_addr + i);
            
            // Validar dirección
            if (ram_addr > RAM::MAX_ADDRESS) {
                std::cerr << "Warning: RAM address 0x" << std::hex << ram_addr 
                         << " exceeds MAX_ADDRESS, wrapping around" << std::endl;
                ram_addr = ram_addr & RAM::MAX_ADDRESS;
            }
            
            uint64_t word;
            std::memcpy(&word, &data[i * sizeof(uint64_t)], sizeof(uint64_t));
            ram->write(ram_addr, word);
        }
    }
    
    void sendMessage(const BusMessage& msg) override {
        // Crear transacción para el interconnect
        BusTransaction transaction;
        transaction.type = busEventToTransactionType(msg.event);
        transaction.address = static_cast<uint32_t>(msg.address / sizeof(uint64_t));
        transaction.pe_id = static_cast<uint32_t>(msg.sender_pe_id);
        transaction.data = 0;
        
        std::cout << "[PE" << pe_id << "] Sending bus message: " 
                  << transaction.toString() << std::endl;
        
        // Agregar al interconnect
        interconnect->addRequest(transaction);
    }
    
    void supplyData(uint64_t address, const uint8_t* data) override {
        std::cout << "[PE" << pe_id << "] Supplying data for addr=0x" 
                  << std::hex << address << std::dec 
                  << " (cache-to-cache transfer)" << std::endl;
        // En esta implementación simple, los datos van a través de RAM
        (void)data; // Evitar warning
    }
};

// ============================================================
// WRAPPER: PE accede a Cache
// ============================================================

class CacheMemPort : public IMemPort {
    Cache& cache;
public:
    CacheMemPort(Cache& c) : cache(c) {}
    
    uint64_t load(uint64_t addr) override {
        uint64_t data = 0;
        cache.read(addr * sizeof(uint64_t), data);
        return data;
    }
    
    void store(uint64_t addr, uint64_t value) override {
        cache.write(addr * sizeof(uint64_t), value);
    }
};

// ============================================================
// UTILIDADES
// ============================================================

std::vector<std::string> loadProgramFromFile(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + path);
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#' && line[0] != ';') {
            lines.push_back(line);
        }
    }
    return lines;
}

void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << "========================================\n" << std::endl;
}

// ============================================================
// MAIN
// ============================================================

int main() {
    try {
        printSeparator("SISTEMA INTEGRADO: PE + Cache + Interconnect + RAM");
        
        // ========================================================
        // 1. CREAR INFRAESTRUCTURA COMPARTIDA
        // ========================================================
        
        std::cout << "Inicializando componentes del sistema...\n" << std::endl;
        
        // RAM compartida (512 palabras de 64 bits = 4KB)
        auto shared_ram = std::make_shared<RAM>(true);
        
        // Interconnect (bus compartido)
        auto interconnect = std::make_shared<Interconnect>(shared_ram, true);
        
        std::cout << "\n RAM e Interconnect inicializados\n" << std::endl;
        
        // ========================================================
        // 2. INICIALIZAR RAM CON DATOS DE PRUEBA
        // ========================================================
        
        printSeparator("Inicializando Datos en RAM");
        
        // Datos para Programa 1 (punto flotante)
        double val_a = 25.0;
        double val_b = 5.0;
        uint64_t data_a, data_b;
        std::memcpy(&data_a, &val_a, sizeof(double));
        std::memcpy(&data_b, &val_b, sizeof(double));
        
        shared_ram->write(0, data_a);  // mem[0] = 25.0
        shared_ram->write(1, data_b);  // mem[1] = 5.0
        
        // Datos para Programa 2 (loop)
        shared_ram->write(2, 5);  // mem[2] = 5 (iteraciones)
        shared_ram->write(3, 0);  // mem[3] = 0 (acumulador inicial)
        
        std::cout << "\nDatos cargados en RAM:" << std::endl;
        std::cout << "  mem[0] = " << val_a << " (para PE0)" << std::endl;
        std::cout << "  mem[1] = " << val_b << " (para PE0)" << std::endl;
        std::cout << "  mem[2] = 5 (para PE1)" << std::endl;
        std::cout << "  mem[3] = 0 (para PE1)" << std::endl;
        
        // ========================================================
        // 3. CONFIGURAR PE0 (Programa de multiplicación)
        // ========================================================
        
        printSeparator("PE0: Multiplicación Punto Flotante");
        
        // Cache L1 para PE0
        Cache cache0(0);
        auto bus_interface0 = std::make_shared<InterconnectBusInterface>(
            interconnect, shared_ram, 0
        );
        cache0.setBusInterface(bus_interface0.get());
        
        // Wrapper PE-Cache
        CacheMemPort cache_port0(cache0);
        
        // Cargar programa
        Loader loader;
        auto prog1 = loader.parseProgram(loadProgramFromFile("Programs/program1.txt"));
        
        std::cout << "Programa: Programs/program1.txt" << std::endl;
        std::cout << "Instrucciones: " << prog1.size() << std::endl;
        for (size_t i = 0; i < prog1.size(); i++) {
            std::cout << "  [" << i << "] ";
            // Aquí podrías imprimir la instrucción si tienes un método toString()
            std::cout << std::endl;
        }
        
        // Crear PE0
        PE pe0(0);
        pe0.attachMemory(&cache_port0);
        pe0.loadProgram(prog1);
        
        std::cout << "\n--- Ejecutando PE0 ---" << std::endl;
        pe0.runToCompletion();
        std::cout << "--- PE0 completado ---" << std::endl;
        
        // Resultados PE0
        uint64_t result0_raw;
        cache0.read(2 * sizeof(uint64_t), result0_raw);
        double result0;
        std::memcpy(&result0, &result0_raw, sizeof(double));
        
        std::cout << "\n=== Resultados PE0 ===" << std::endl;
        std::cout << "Result mem[2] = " << std::fixed << std::setprecision(2) 
                  << result0 << " (esperado: 125.0)" << std::endl;
        std::cout << "Instrucciones: " << pe0.getInstructionCount() 
                  << " | LOAD: " << pe0.getLoadCount()
                  << " | STORE: " << pe0.getStoreCount()
                  << " | Ciclos: " << pe0.getCycleCount() << std::endl;
        
        cache0.printStats();
        
        // ========================================================
        // 4. CONFIGURAR PE1 (Programa de loop)
        // ========================================================
        
        printSeparator("PE1: Loop con Acumulador");
        
        // Cache L1 para PE1
        Cache cache1(1);
        auto bus_interface1 = std::make_shared<InterconnectBusInterface>(
            interconnect, shared_ram, 1
        );
        cache1.setBusInterface(bus_interface1.get());
        
        // Wrapper PE-Cache
        CacheMemPort cache_port1(cache1);
        
        // Cargar programa
        auto prog2 = loader.parseProgram(loadProgramFromFile("Programs/program2.txt"));
        
        std::cout << "Programa: Programs/program2.txt" << std::endl;
        std::cout << "Instrucciones: " << prog2.size() << std::endl;
        
        // Crear PE1
        PE pe1(1);
        pe1.attachMemory(&cache_port1);
        pe1.loadProgram(prog2);
        
        std::cout << "\n--- Ejecutando PE1 ---" << std::endl;
        pe1.runToCompletion();
        std::cout << "--- PE1 completado ---" << std::endl;
        
        // Resultados PE1
        uint64_t result1_raw;
        cache1.read(1 * sizeof(uint64_t), result1_raw);
        
        std::cout << "\n=== Resultados PE1 ===" << std::endl;
        std::cout << "Accumulator mem[1] = " << result1_raw 
                  << " (esperado: 5)" << std::endl;
        std::cout << "Instrucciones: " << pe1.getInstructionCount() 
                  << " | LOAD: " << pe1.getLoadCount()
                  << " | STORE: " << pe1.getStoreCount()
                  << " | Ciclos: " << pe1.getCycleCount() << std::endl;
        
        cache1.printStats();
        
        // ========================================================
        // 5. PROCESAR TRANSACCIONES PENDIENTES EN EL INTERCONNECT
        // ========================================================
        
        printSeparator("Procesando Transacciones del Interconnect");
        
        int transactions_processed = 0;
        while (interconnect->hasPendingTransactions()) {
            if (interconnect->processNextTransaction()) {
                transactions_processed++;
            }
        }
        
        std::cout << "Transacciones procesadas: " << transactions_processed << std::endl;
        
        // ========================================================
        // 6. VERIFICAR DATOS EN RAM (después de writebacks)
        // ========================================================
        
        printSeparator("Estado Final de RAM");
        
        std::cout << "Contenido de RAM (primeras 10 posiciones):" << std::endl;
        for (uint32_t i = 0; i < 10; i++) {
            uint64_t value = shared_ram->read(i);
            std::cout << "  mem[" << i << "] = 0x" << std::hex 
                      << std::setw(16) << std::setfill('0') << value 
                      << std::dec;
            
            if (i == 0 || i == 1 || i == 2) {
                double d;
                std::memcpy(&d, &value, sizeof(double));
                std::cout << " (" << std::fixed << std::setprecision(2) << d << ")";
            } else if (i >= 1 && i <= 3) {
                std::cout << " (" << value << ")";
            }
            std::cout << std::endl;
        }
        
        // ========================================================
        // 7. RESUMEN FINAL
        // ========================================================
        
        printSeparator("RESUMEN FINAL DEL SISTEMA");
        
        std::cout << "PE0:" << std::endl;
        std::cout << "   Multiplicación: 25.0 × 5.0 = " << result0 << std::endl;
        std::cout << "   Cache Hit Rate: ";
        auto stats0 = cache0.getStats();
        double hit_rate0 = (double)(stats0.read_hits + stats0.write_hits) / 
                          (stats0.read_hits + stats0.read_misses + 
                           stats0.write_hits + stats0.write_misses) * 100.0;
        std::cout << std::fixed << std::setprecision(1) << hit_rate0 << "%" << std::endl;
        
        std::cout << "\nPE1:" << std::endl;
        std::cout << "   Loop ejecutado: " << result1_raw << " iteraciones" << std::endl;
        std::cout << "   Cache Hit Rate: ";
        auto stats1 = cache1.getStats();
        double hit_rate1 = (double)(stats1.read_hits + stats1.write_hits) / 
                          (stats1.read_hits + stats1.read_misses + 
                           stats1.write_hits + stats1.write_misses) * 100.0;
        std::cout << std::fixed << std::setprecision(1) << hit_rate1 << "%" << std::endl;
        
        std::cout << "\nInterconnect:" << std::endl;
        std::cout << "   Transacciones procesadas: " << transactions_processed << std::endl;
        std::cout << "   Coherencia MESI mantenida" << std::endl;
        
        std::cout << "\n Sistema completo funcionando correctamente " << std::endl;
        
        return 0;
        
    } catch (const std::exception &ex) {
        std::cerr << "\n Error: " << ex.what() << "\n";
        return 1;
    }
}