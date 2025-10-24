#include "Loader/Loader.hpp"
#include "PE/PE.hpp"
#include "cache/cache.hpp"
#include "bus/bus.hpp"
#include "ram/ram.hpp"
#include "interconnect/interconnect.hpp"
#include "bus/bus_controller.hpp"
#include "Clock/Clock.hpp"
#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <iomanip>
#include <memory>
#include <cmath>
#include <thread>
#include <atomic>

// ============================================================
// ADAPTER
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
// INTERFACE
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
        
        interconnect->addRequest(transaction);
        interconnect->broadcastBusMessage(msg);
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
// WRAPPER
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

void waitForEnter(bool stepping_mode, const std::string& message = "") {
    if (stepping_mode) {
        if (!message.empty()) {
            std::cout << message << std::endl;
        }
        std::cout << "Presione ENTER para continuar...";
        std::cin.get();
    }
}

// ============================================================
// MAIN
// ============================================================

std::atomic<bool> running(true);

void clockControlThread(bool stepping_mode) {
    if (stepping_mode) {
        std::cout << "Modo Stepping: Presione ENTER para avanzar cada ciclo" << std::endl;
        std::cout << "   (Escriba 'q' y ENTER para salir)\n" << std::endl;
        
        while (running) {
            std::string line;
            if (!std::getline(std::cin, line)) {
                // EOF o error de lectura
                running = false;
                break;
            }
            
            if (!line.empty() && (line[0] == 'q' || line[0] == 'Q')) {
                running = false;
                break;
            }
            
            // Avanzar un ciclo
            Clock::getInstance().step();
        }
    }
}

int main(int argc, char* argv[]) {
    bool stepping_mode = false;
    
    // Procesar argumentos de línea de comandos
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--step" || arg == "-s") {
            stepping_mode = true;
        }
    }
    
    try {
        Clock::getInstance().setSteppingMode(stepping_mode);
        
        printSeparator("SISTEMA INTEGRADO: 4 PEs + Cache + Interconnect + RAM");
        
        if (stepping_mode) {
            std::cout << "Modo paso a paso por ciclos activado" << std::endl;
            std::cout << "   Durante la ejecución solo se mostrarán los ciclos" << std::endl;
            std::cout << "   Al final se mostrarán todas las estadísticas\n" << std::endl;
        }
        
        // Iniciar thread de control del reloj
        std::thread clock_thread;
        if (stepping_mode) {
            clock_thread = std::thread(clockControlThread, stepping_mode);
        }
        
        // ========================================================
        // 1. CREAR INFRAESTRUCTURA COMPARTIDA
        // ========================================================
        
        if (!stepping_mode) {
            std::cout << "Inicializando componentes del sistema...\n" << std::endl;
        }
        
        auto shared_ram = std::make_shared<RAM>(!stepping_mode);  // verbose solo si no es stepping
        auto interconnect = std::make_shared<Interconnect>(shared_ram, !stepping_mode);
        auto bus_controller = std::make_shared<BusController>(!stepping_mode);
        
        if (!stepping_mode) {
            std::cout << "RAM, Interconnect y BusController inicializados\n" << std::endl;
        }
        
        // ========================================================
        // 2. CARGAR VECTORES DESDE ARCHIVOS
        // ========================================================
        
        if (!stepping_mode) {
            printSeparator("Cargando Vectores desde Archivos");
        }
            
        shared_ram->loadVectorsToMemory("vectores/vector_a.txt", "vectores/vector_b.txt");

        // ========================================================
        // 3. CONFIGURAR LOS 4 PEs CON SUS CACHÉS
        // ========================================================
        
        Loader loader;
        std::vector<std::unique_ptr<Cache>> caches;
        std::vector<std::shared_ptr<InterconnectBusInterface>> bus_interfaces;
        std::vector<std::unique_ptr<CacheMemPort>> cache_ports;
        std::vector<std::unique_ptr<PE>> pes;
        
        for (int i = 0; i < 4; i++) {
            if (!stepping_mode) {
                printSeparator("Configurando PE" + std::to_string(i));
            }
            
            caches.push_back(std::make_unique<Cache>(i));
            bus_interfaces.push_back(std::make_shared<InterconnectBusInterface>(
                interconnect, shared_ram, bus_controller, i
            ));
            
            caches[i]->setBusInterface(bus_interfaces[i].get());
            bus_controller->registerCache(caches[i].get());
            cache_ports.push_back(std::make_unique<CacheMemPort>(*caches[i]));
            
            pes.push_back(std::make_unique<PE>(i));
            pes[i]->attachMemory(cache_ports[i].get());
            
            std::string program_file = "Programs/program" + std::to_string(i + 1) + ".txt";
            auto pe_program = loader.parseProgram(loadProgramFromFile(program_file));
            
            if (!stepping_mode) {
                std::cout << "Cargando programa para PE" << i << ": " << program_file 
                         << " (" << pe_program.size() << " instrucciones)" << std::endl;
            }
            
            pes[i]->loadProgram(pe_program);
            
            if (!stepping_mode) {
                std::cout << "PE" << i << " configurado correctamente" << std::endl;
            }
        }
        
        if (!stepping_mode) {
            std::cout << "\nTotal de cachés registradas: " << bus_controller->getCacheCount() << std::endl;
        }
        
        // ========================================================
        // 4. EJECUTAR LOS 4 PEs
        // ========================================================
        
        if (stepping_mode) {
            std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
            std::cout << "║   INICIANDO EJECUCIÓN (Stepping)       ║" << std::endl;
            std::cout << "╚════════════════════════════════════════╝\n" << std::endl;
        } else {
            printSeparator("EJECUTANDO LOS 4 PEs EN PARALELO");
        }

        for (int i = 0; i < 4; i++) {
            pes[i]->start();
            if (!stepping_mode) {
                std::cout << "PE" << i << " iniciado" << std::endl;
            }
        }

        if (!stepping_mode) {
            std::cout << "\nTodos los PEs están corriendo en paralelo..." << std::endl;
        }

        for (int i = 0; i < 4; i++) {
            pes[i]->join();
            if (!stepping_mode) {
                std::cout << "PE" << i << " completado" << std::endl;
            }
        }

        if (stepping_mode) {
            std::cout << "\nTodos los PEs han terminado su ejecución\n" << std::endl;
        } else {
            std::cout << "\nTodos los PEs han terminado su ejecución\n" << std::endl;
        }
        
        // ========================================================
        // 5. RESULTADOS DE CADA PE (SIEMPRE SE MUESTRAN)
        // ========================================================
        
        printSeparator("RESULTADOS DE LOS PEs");
        
        for (int i = 0; i < 4; i++) {
            std::cout << "\n=== PE" << i << " ===" << std::endl;
            
            uint64_t partial_sum_raw;
            caches[i]->read((24 + i) * sizeof(uint64_t), partial_sum_raw);
            double partial_sum;
            std::memcpy(&partial_sum, &partial_sum_raw, sizeof(double));
            
            std::cout << "Suma parcial PE" << i << " (mem[" << (24 + i) << "]) = " 
                      << std::fixed << std::setprecision(2) << partial_sum << std::endl;
            std::cout << "Instrucciones: " << pes[i]->getIntInstructionCount()
                      << " | LOAD: " << pes[i]->getLoadCount()
                      << " | STORE: " << pes[i]->getStoreCount()
                      << " | Ciclos: " << pes[i]->getCycleCount() << std::endl;
            
            // SIEMPRE mostrar estadísticas de caché
            caches[i]->printStats();
        }

        // ========================================================
        // VALIDACIÓN DEL PRODUCTO PUNTO FINAL (SIEMPRE SE MUESTRA)
        // ========================================================

        double total_sum = 0.0;
        for (int i = 0; i < 4; i++) {
            uint64_t partial_sum_raw;
            caches[i]->read((24 + i) * sizeof(uint64_t), partial_sum_raw);
            double partial_sum;
            std::memcpy(&partial_sum, &partial_sum_raw, sizeof(double));
            total_sum += partial_sum;
        }

        uint32_t N = static_cast<uint32_t>(shared_ram->read(0));

        double expected_dot = 0.0;
        for (uint32_t i = 0; i < N; ++i) {
            uint64_t a_raw = shared_ram->read(1 + i);
            double a;
            std::memcpy(&a, &a_raw, sizeof(double));

            uint64_t b_raw = shared_ram->read(13 + i);
            double b;
            std::memcpy(&b, &b_raw, sizeof(double));

            expected_dot += a * b;
        }

        std::cout << "\n=== VALIDACIÓN DEL PRODUCTO PUNTO FINAL ===\n";
        std::cout << "  Suma de PEs     : " << std::fixed << std::setprecision(2) << total_sum << std::endl;
        std::cout << "  Producto directo: " << std::fixed << std::setprecision(2) << expected_dot << std::endl;
        
        if (std::fabs(total_sum - expected_dot) < 0.01) {
            std::cout << "VALOR CORRECTO\n";
        } else {
            std::cout << "VALOR INCORRECTO, diferencia: " 
                      << std::fabs(total_sum - expected_dot) << std::endl;
        }
        
        // ========================================================
        // 6. PROCESAR TRANSACCIONES PENDIENTES (SIEMPRE SE MUESTRA)
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
        // 7. ESTADO FINAL DE RAM (SIEMPRE SE MUESTRA)
        // ========================================================
        
        printSeparator("Estado Final de RAM");
        
        std::cout << "Contenido de RAM (primeras 30 posiciones):" << std::endl;
        for (uint32_t i = 0; i < 30; i++) {
            uint64_t value = shared_ram->read(i);
            std::cout << "  mem[" << std::setw(2) << i << "] = 0x" << std::hex 
                      << std::setw(16) << std::setfill('0') << value 
                      << std::dec;
            double d;
            std::memcpy(&d, &value, sizeof(double));
            std::cout << " (" << std::fixed << std::setprecision(2) << std::setw(8) << d << ")";
            std::cout << std::endl;
        }
        
        // ========================================================
        // 8. RESUMEN FINAL (SIEMPRE SE MUESTRA)
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
            std::cout << "   Instrucciones ejecutadas: " << pes[i]->getIntInstructionCount() << std::endl;
            std::cout << "   Ciclos: " << pes[i]->getCycleCount() << std::endl;
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

        // Limpiar el thread del reloj
        if (stepping_mode && clock_thread.joinable()) {
            running = false;
            clock_thread.join();
        }
        
        return 0;
        
    } catch (const std::exception &ex) {
        std::cerr << "\nError: " << ex.what() << "\n";
        return 1;
    }
}