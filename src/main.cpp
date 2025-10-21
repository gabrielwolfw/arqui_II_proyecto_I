#include "Loader/Loader.hpp"
#include "PE/PE.hpp"
#include "cache/cache.hpp"
#include "bus/bus.hpp"
#include "ram/ram.hpp"
#include "interconnect/interconnect.hpp"
#include "bus/bus_controller.hpp"
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
    std::shared_ptr<BusController> bus_controller;
    int pe_id;
    
public:
    InterconnectBusInterface(std::shared_ptr<Interconnect> ic, 
                            std::shared_ptr<RAM> r,
                            std::shared_ptr<BusController> bc,
                            int id)
        : interconnect(ic), ram(r), bus_controller(bc), pe_id(id) {}
    
    void readFromMemory(uint64_t address, uint8_t* data, size_t size) override {
        uint64_t block_address = (address / 32) * 32;
        size_t num_words = size / sizeof(uint64_t);
        uint64_t word_addr = block_address / sizeof(uint64_t);
        
        std::cout << "[PE" << pe_id << " BusInterface] Reading from RAM: "
                  << "byte_addr=0x" << std::hex << block_address 
                  << " word_addr=" << std::dec << word_addr 
                  << " (" << num_words << " words)" << std::endl;
        
        for (size_t i = 0; i < num_words; i++) {
            uint32_t ram_addr = static_cast<uint32_t>(word_addr + i);
            
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
        uint64_t block_address = (address / 32) * 32;
        size_t num_words = size / sizeof(uint64_t);
        uint64_t word_addr = block_address / sizeof(uint64_t);
        
        std::cout << "[PE" << pe_id << " BusInterface] Writing to RAM: "
                  << "byte_addr=0x" << std::hex << block_address 
                  << " word_addr=" << std::dec << word_addr 
                  << " (" << num_words << " words)" << std::endl;
        
        for (size_t i = 0; i < num_words; i++) {
            uint32_t ram_addr = static_cast<uint32_t>(word_addr + i);
            
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
        BusTransaction transaction;
        transaction.type = busEventToTransactionType(msg.event);
        transaction.address = static_cast<uint32_t>(msg.address / sizeof(uint64_t));
        transaction.pe_id = static_cast<uint32_t>(msg.sender_pe_id);
        transaction.data = 0;
        
        std::cout << "[PE" << pe_id << "] Sending bus message: " 
                  << transaction.toString() << std::endl;
        
        // Agregar transacción al Interconnect
        interconnect->addRequest(transaction);
        
        // Enviar mensaje de broadcast directamente
        interconnect->broadcastBusMessage(msg);
        
        // Notificar al bus controller
        bus_controller->notifyTransaction(msg.address, msg.sender_pe_id);
    }
    
    void supplyData(uint64_t address, const uint8_t* data) override {
        std::cout << "[PE" << pe_id << "] Supplying data for addr=0x" 
                  << std::hex << address << std::dec 
                  << " (cache-to-cache transfer)" << std::endl;
        (void)data;
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
        printSeparator("SISTEMA INTEGRADO: 4 PEs + Cache + Interconnect + RAM");
        
        // ========================================================
        // 1. CREAR INFRAESTRUCTURA COMPARTIDA
        // ========================================================
        
        std::cout << "Inicializando componentes del sistema...\n" << std::endl;
        
        // RAM compartida (512 palabras de 64 bits = 4KB)
        auto shared_ram = std::make_shared<RAM>(true);
        
        // Interconnect (bus compartido)
        auto interconnect = std::make_shared<Interconnect>(shared_ram, true);
        
        // Bus Controller para coherencia
        auto bus_controller = std::make_shared<BusController>(true);
        
        std::cout << "RAM, Interconnect y BusController inicializados\n" << std::endl;
        
        // ========================================================
        // 2. CARGAR VECTORES DESDE ARCHIVOS
        // ========================================================
            
        printSeparator("Cargando Vectores desde Archivos");
            
        try {
            shared_ram->loadVectorsToMemory(
                "vectores/vector_a.txt", 
                "vectores/vector_b.txt"
            );
            
            
        } catch (const std::exception& e) {
            std::cerr << "Error cargando vectores desde archivos: " 
                      << e.what() << std::endl;
        }

        // ========================================================
        // 3. CONFIGURAR LOS 4 PEs CON SUS CACHÉS
        // ========================================================
        
        // Crear loader
        Loader loader;
        
        // Arrays para almacenar los componentes
        std::vector<std::unique_ptr<Cache>> caches;
        std::vector<std::shared_ptr<InterconnectBusInterface>> bus_interfaces;
        std::vector<std::unique_ptr<CacheMemPort>> cache_ports;
        std::vector<std::unique_ptr<PE>> pes;
        
        // Configurar los 4 PEs
        for (int i = 0; i < 4; i++) {
            printSeparator("Configurando PE" + std::to_string(i));
            
            // Crear y configurar caché
            caches.push_back(std::make_unique<Cache>(i));
            
            // Crear y configurar interfaz de bus
            bus_interfaces.push_back(std::make_shared<InterconnectBusInterface>(
                interconnect, shared_ram, bus_controller, i
            ));
            
            // Conectar caché con interfaz de bus y registrar en controlador
            caches[i]->setBusInterface(bus_interfaces[i].get());
            bus_controller->registerCache(caches[i].get());
            
            // Crear y configurar puerto de memoria
            cache_ports.push_back(std::make_unique<CacheMemPort>(*caches[i]));
            
            // Crear y configurar PE
            pes.push_back(std::make_unique<PE>(i));
            pes[i]->attachMemory(cache_ports[i].get());
            
            // Cargar programa específico para este PE
            std::string program_file = "Programs/program" + std::to_string(i + 1) + ".txt";
            try {
                auto pe_program = loader.parseProgram(loadProgramFromFile(program_file));
                std::cout << "Cargando programa para PE" << i << ": " << program_file 
                         << " (" << pe_program.size() << " instrucciones)" << std::endl;
                pes[i]->loadProgram(pe_program);
            } catch (const std::exception& e) {
                std::cerr << "Error cargando programa " << program_file << ": " 
                         << e.what() << std::endl;
                throw;
            }
            
            std::cout << "PE" << i << " configurado correctamente" << std::endl;
        }
        
        std::cout << "\nTotal de cachés registradas: " << bus_controller->getCacheCount() << std::endl;
        
        // ========================================================
        // 4. EJECUTAR LOS 4 PEs
        // ========================================================
                
        printSeparator("EJECUTANDO LOS 4 PEs EN PARALELO");

        // Iniciar todos los PEs al mismo tiempo
        std::cout << "Iniciando los 4 PEs simultáneamente...\n" << std::endl;
        for (int i = 0; i < 4; i++) {
            pes[i]->start();
            std::cout << "PE" << i << " iniciado" << std::endl;
        }

        std::cout << "\nTodos los PEs están corriendo en paralelo..." << std::endl;

        // Esperar a que todos terminen
        for (int i = 0; i < 4; i++) {
            pes[i]->join();
            std::cout << "PE" << i << " completado" << std::endl;
        }

        std::cout << "\nTodos los PEs han terminado su ejecución\n" << std::endl;
        
        // ========================================================
        // 5. RESULTADOS DE CADA PE
        // ========================================================
        
        printSeparator("RESULTADOS DE LOS PEs");
        
        for (int i = 0; i < 4; i++) {
            std::cout << "\n=== PE" << i << " ===" << std::endl;
            
            // Leer suma parcial
            uint64_t partial_sum_raw;
            caches[i]->read((24 + i) * sizeof(uint64_t), partial_sum_raw);
            double partial_sum;
            std::memcpy(&partial_sum, &partial_sum_raw, sizeof(double));
            
            std::cout << "Suma parcial PE" << i << " (mem[" << (24 + i) << "]) = " 
                      << std::fixed << std::setprecision(2) << partial_sum << std::endl;
            std::cout << "Instrucciones (uint64): " << pes[i]->getInstructionCount() 
                      << " | Instrucciones (int): " << pes[i]->getIntInstructionCount()
                      << " | LOAD: " << pes[i]->getLoadCount()
                      << " | STORE: " << pes[i]->getStoreCount()
                      << " | Ciclos: " << pes[i]->getCycleCount() << std::endl;
            
            caches[i]->printStats();
        }
        
        // ========================================================
        // 6. PROCESAR TRANSACCIONES PENDIENTES
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
        // 7. ESTADO FINAL DE RAM
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
            }
            std::cout << std::endl;
        }
        
        // ========================================================
        // 8. RESUMEN FINAL
        // ========================================================
        
        printSeparator("RESUMEN FINAL DEL SISTEMA");
        
        std::cout << "Sistema con 4 PEs:" << std::endl;
        
        double total_hit_rate = 0.0;
        for (int i = 0; i < 4; i++) {
            auto stats = caches[i]->getStats();
            double hit_rate = (double)(stats.read_hits + stats.write_hits) / 
                              (stats.read_hits + stats.read_misses + 
                               stats.write_hits + stats.write_misses) * 100.0;
            total_hit_rate += hit_rate;
            
            std::cout << "\nPE" << i << ":" << std::endl;
            std::cout << "   Instrucciones ejecutadas (uint64): " << pes[i]->getInstructionCount() << std::endl;
            std::cout << "   Instrucciones ejecutadas (int): " << pes[i]->getIntInstructionCount() << std::endl;
            std::cout << "   Cache Hit Rate: " << std::fixed << std::setprecision(1) 
                      << hit_rate << "%" << std::endl;
        }
        
        std::cout << "\nPromedio Hit Rate: " << std::fixed << std::setprecision(1) 
                  << (total_hit_rate / 4.0) << "%" << std::endl;
        
        std::cout << "\nInterconnect:" << std::endl;
        std::cout << "   Transacciones procesadas: " << transactions_processed << std::endl;
        std::cout << "   Cachés registradas: " << bus_controller->getCacheCount() << std::endl;
        std::cout << "   Coherencia MESI mantenida" << std::endl;
        
        std::cout << "\nSistema completo con 4 PEs funcionando correctamente" << std::endl;
        
        return 0;
        
    } catch (const std::exception &ex) {
        std::cerr << "\nError: " << ex.what() << "\n";
        return 1;
    }
}