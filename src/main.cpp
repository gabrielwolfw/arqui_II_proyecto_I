#include "Loader.hpp"
#include "PE.hpp"
#include "MockMemPort.hpp"
#include "cache.hpp"
#include <iostream>
#include <cstring>
#include <cmath>
#include <fstream>
#include <vector>
#include <iomanip>

// ============================================================
// INTERFACES PARA INTEGRAR CACHE CON MEMORIA PRINCIPAL
// ============================================================

// Interface entre Cache y MockMemPort (memoria principal)
class CacheBusInterface : public IBusInterface {
    MockMemPort& main_memory;
public:
    CacheBusInterface(MockMemPort& mem) : main_memory(mem) {}
    
    void readFromMemory(uint64_t address, uint8_t* data, size_t size) override {
        // Cache pasa direcciones en BYTES, MockMemPort usa índices de uint64_t
        // Necesitamos alinear a bloques de cache completos
        
        size_t num_words = size / sizeof(uint64_t);
        uint64_t word_addr = address / sizeof(uint64_t); // Convertir bytes a índice de palabra
        
        std::cout << "[BusInterface] Reading from memory: byte_addr=0x" << std::hex << address 
                  << " word_addr=" << std::dec << word_addr 
                  << " size=" << size << " bytes (" << num_words << " words)" << std::endl;
        
        for (size_t i = 0; i < num_words; i++) {
            uint64_t word = main_memory.load(word_addr + i);
            std::memcpy(&data[i * sizeof(uint64_t)], &word, sizeof(uint64_t));
            
            std::cout << "  [" << i << "] mem[" << (word_addr + i) << "] = 0x" 
                      << std::hex << word << std::dec << std::endl;
        }
    }
    
    void writeToMemory(uint64_t address, const uint8_t* data, size_t size) override {
        // Cache pasa direcciones en BYTES, MockMemPort usa índices de uint64_t
        
        size_t num_words = size / sizeof(uint64_t);
        uint64_t word_addr = address / sizeof(uint64_t); // Convertir bytes a índice de palabra
        
        std::cout << "[BusInterface] Writing to memory: byte_addr=0x" << std::hex << address 
                  << " word_addr=" << std::dec << word_addr 
                  << " size=" << size << " bytes (" << num_words << " words)" << std::endl;
        
        for (size_t i = 0; i < num_words; i++) {
            uint64_t word;
            std::memcpy(&word, &data[i * sizeof(uint64_t)], sizeof(uint64_t));
            main_memory.store(word_addr + i, word);
            
            std::cout << "  [" << i << "] mem[" << (word_addr + i) << "] = 0x" 
                      << std::hex << word << std::dec << std::endl;
        }
    }
    
    void sendMessage(const BusMessage& msg) override {
        // En modo standalone, no hay otros caches
        (void)msg;
    }
    
    void supplyData(uint64_t address, const uint8_t* data) override {
        // En modo standalone, no se necesita
        (void)address;
        (void)data;
    }
};

// Wrapper para que el PE use el cache para LOAD/STORE
class CacheMemPort : public IMemPort {
    Cache& cache;
public:
    CacheMemPort(Cache& c) : cache(c) {}
    
    uint64_t load(uint64_t addr) override {
        uint64_t data = 0;
        // PE usa índices lógicos (0, 1, 2...), convertir a direcciones en bytes
        cache.read(addr * sizeof(uint64_t), data);
        return data;
    }
    
    void store(uint64_t addr, uint64_t value) override {
        // PE usa índices lógicos (0, 1, 2...), convertir a direcciones en bytes
        cache.write(addr * sizeof(uint64_t), value);
    }
};

// ============================================================
// FUNCIONES AUXILIARES
// ============================================================

std::vector<std::string> loadProgramFromFile(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + path);
    }
    std::string line;
    while (std::getline(file, line)) {
        // Ignorar líneas vacías y comentarios
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
        Loader loader;
        
        // ========================================================
        // PRIMERA PRUEBA: Operaciones con punto flotante
        // ========================================================
        printSeparator("PRUEBA 1: Operaciones Punto Flotante");
        
        // Crear memoria principal
        MockMemPort mem1(16);
        
        // Inicializar datos en memoria principal
        double a = 25.0, b = 5.0;
        std::memcpy(mem1.rawData(), &a, sizeof(double));
        std::memcpy(mem1.rawData() + 1, &b, sizeof(double));
        
        std::cout << "Memoria principal inicializada:" << std::endl;
        std::cout << "  mem[0] = " << a << " (0x" << std::hex 
                  << mem1.load(0) << std::dec << ")" << std::endl;
        std::cout << "  mem[1] = " << b << " (0x" << std::hex 
                  << mem1.load(1) << std::dec << ")" << std::endl;
        
        // Crear cache y conectarlo a memoria principal
        Cache cache1(0);
        CacheBusInterface bus1(mem1);
        cache1.setBusInterface(&bus1);
        
        // Crear wrapper PE-Cache
        CacheMemPort cache_port1(cache1);
        
        // Cargar programa
        auto prog1 = loader.parseProgram(loadProgramFromFile("Programs/program1.txt"));
        std::cout << "\nPrograma cargado: Programs/program1.txt" << std::endl;
        std::cout << "Instrucciones: " << prog1.size() << std::endl;
        
        // Crear PE y ejecutar
        PE pe1(0);
        pe1.attachMemory(&cache_port1);
        pe1.loadProgram(prog1);
        
        std::cout << "\n--- Ejecutando programa ---" << std::endl;
        pe1.runToCompletion();
        std::cout << "--- Ejecución completada ---" << std::endl;
        
        // Leer resultado desde cache (forzar lectura)
        uint64_t result_raw;
        cache1.read(2 * sizeof(uint64_t), result_raw);
        double res;
        std::memcpy(&res, &result_raw, sizeof(double));
        
        std::cout << "\n=== Resultados Prueba 1 ===" << std::endl;
        std::cout << "Result mem[2] = " << std::fixed << std::setprecision(6) << res << std::endl;
        std::cout << "LOAD count: " << pe1.getLoadCount()
                  << " | STORE count: " << pe1.getStoreCount()
                  << " | CYCLES: " << pe1.getCycleCount() << std::endl;
        
        // Mostrar estadísticas del cache
        cache1.printStats();
        
        
        // ========================================================
        // SEGUNDA PRUEBA: Loop con acumulador
        // ========================================================
        printSeparator("PRUEBA 2: Loop con Acumulador");
        
        // Crear memoria principal
        MockMemPort mem2(16);
        mem2.store(2, 5);  // iteraciones en mem[2]
        mem2.store(3, 0);  // acumulador inicial en mem[3]
        
        std::cout << "Memoria principal inicializada:" << std::endl;
        std::cout << "  mem[2] = " << mem2.load(2) << " (iteraciones)" << std::endl;
        std::cout << "  mem[3] = " << mem2.load(3) << " (acumulador inicial)" << std::endl;
        
        // Crear cache y conectarlo a memoria principal
        Cache cache2(1);
        CacheBusInterface bus2(mem2);
        cache2.setBusInterface(&bus2);
        
        // Crear wrapper PE-Cache
        CacheMemPort cache_port2(cache2);
        
        // Cargar programa
        auto prog2 = loader.parseProgram(loadProgramFromFile("Programs/program2.txt"));
        std::cout << "\nPrograma cargado: Programs/program2.txt" << std::endl;
        std::cout << "Instrucciones: " << prog2.size() << std::endl;
        
        // Crear PE y ejecutar
        PE pe2(1);
        pe2.attachMemory(&cache_port2);
        pe2.loadProgram(prog2);
        
        std::cout << "\n--- Ejecutando programa ---" << std::endl;
        pe2.runToCompletion();
        std::cout << "--- Ejecución completada ---" << std::endl;
        
        // Leer resultado desde cache
        uint64_t acc_raw;
        cache2.read(1 * sizeof(uint64_t), acc_raw);
        
        std::cout << "\n=== Resultados Prueba 2 ===" << std::endl;
        std::cout << "Accumulator in mem[1] = " << acc_raw << std::endl;
        std::cout << "LOAD count: " << pe2.getLoadCount()
                  << " | STORE count: " << pe2.getStoreCount()
                  << " | CYCLES: " << pe2.getCycleCount() << std::endl;
        
        // Mostrar estadísticas del cache
        cache2.printStats();
        
        // Verificar valor en memoria principal (después de writeback si ocurre)
        std::cout << "\nValor en memoria principal mem[1] = " << mem2.load(1) << std::endl;
        
        
        // ========================================================
        // RESUMEN FINAL
        // ========================================================
        printSeparator("RESUMEN DE SIMULACIÓN");
        
        std::cout << "PE0 (Prueba 1):" << std::endl;
        std::cout << "  - Instrucciones ejecutadas: " << pe1.getInstructionCount() << std::endl;
        std::cout << "  - Resultado final: " << res << std::endl;
        
        std::cout << "\nPE1 (Prueba 2):" << std::endl;
        std::cout << "  - Instrucciones ejecutadas: " << pe2.getInstructionCount() << std::endl;
        std::cout << "  - Acumulador final: " << acc_raw << std::endl;
        
        std::cout << "\n✓ Todas las pruebas completadas exitosamente" << std::endl;
        
    } catch (const std::exception &ex) {
        std::cerr << "\n❌ Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}