#include "Loader.hpp"
#include "PE.hpp"
#include "cache.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>

// Wrapper para que el PE use el cache para LOAD/STORE
class CacheMemPort : public IMemPort {
    Cache& cache;
public:
    CacheMemPort(Cache& c) : cache(c) {}
    
    uint64_t load(uint64_t addr) override {
        uint64_t data = 0;
        // Multiplicar por 8 para obtener dirección en bytes (uint64_t = 8 bytes)
        cache.read(addr * sizeof(uint64_t), data);
        return data;
    }
    
    void store(uint64_t addr, uint64_t value) override {
        // Multiplicar por 8 para obtener dirección en bytes
        cache.write(addr * sizeof(uint64_t), value);
    }
};

// Helper para convertir double a uint64_t (representación de bits)
static uint64_t doubleToUint64(double d) {
    uint64_t x;
    std::memcpy(&x, &d, sizeof(double));
    return x;
}

// Helper para convertir uint64_t a double
static double uint64ToDouble(uint64_t x) {
    double d;
    std::memcpy(&d, &x, sizeof(double));
    return d;
}

int main() {
    try {
        std::cout << "=== Inicializando simulación PE + Cache ===" << std::endl;
        std::cout << "Nota: Direcciones lógicas del PE se mapean a direcciones físicas (x8 bytes)\n" << std::endl;
        
        // 1. Crear una instancia de Cache
        Cache cache(0); // PE ID 0, modo standalone
        
        // 2. Crear el wrapper para que el PE use el cache
        CacheMemPort cacheMemPort(cache);
        
        // 3. **IMPORTANTE**: Inicializar memoria con datos de prueba
        //    Las direcciones del PE son índices lógicos (0, 1, 2...)
        //    Se mapean a direcciones físicas (0, 8, 16... bytes)
        std::cout << "=== Inicializando memoria con datos de prueba ===" << std::endl;
        
        // Convertir doubles a uint64_t para almacenar como bits
        double val0 = 5.5;
        double val1 = 2.0;
        double val2 = 0.0;
        uint64_t mem0 = doubleToUint64(val0);
        uint64_t mem1 = doubleToUint64(val1);
        uint64_t mem2 = doubleToUint64(val2);
        
        // Escribir en direcciones físicas (en bytes)
        cache.write(0 * sizeof(uint64_t), mem0);  // addr física 0: memoria[0] = 5.5
        cache.write(1 * sizeof(uint64_t), mem1);  // addr física 8: memoria[1] = 2.0
        cache.write(2 * sizeof(uint64_t), mem2);  // addr física 16: memoria[2] = 0.0
        
        std::cout << "Memoria inicializada (direcciones físicas en bytes):" << std::endl;
        std::cout << "  memoria[0] @ 0x00 = " << val0 << " (bits: 0x" 
                  << std::hex << mem0 << std::dec << ")" << std::endl;
        std::cout << "  memoria[1] @ 0x08 = " << val1 << " (bits: 0x" 
                  << std::hex << mem1 << std::dec << ")" << std::endl;
        std::cout << "  memoria[2] @ 0x10 = " << val2 << " (bits: 0x" 
                  << std::hex << mem2 << std::dec << ")" << std::endl;
        
        // Verificar que los datos se escribieron correctamente
        std::cout << "\n=== Verificando escritura inicial ===" << std::endl;
        uint64_t test0, test1, test2;
        cache.read(0 * sizeof(uint64_t), test0);
        cache.read(1 * sizeof(uint64_t), test1);
        cache.read(2 * sizeof(uint64_t), test2);
        std::cout << "Verificación lectura memoria[0]: " << uint64ToDouble(test0) << std::endl;
        std::cout << "Verificación lectura memoria[1]: " << uint64ToDouble(test1) << std::endl;
        std::cout << "Verificación lectura memoria[2]: " << uint64ToDouble(test2) << std::endl;
        
        // 4. Crear un programa de ejemplo para el PE
        std::cout << "\n=== Programa a ejecutar ===" << std::endl;
        std::vector<std::string> program_lines = {
            "LOAD REG1, 0",           // Carga memoria[0] (5.5) en REG1
            "LOAD REG2, 1",           // Carga memoria[1] (2.0) en REG2
            "FMUL REG3, REG1, REG2",  // REG3 = REG1 * REG2 = 11.0
            "STORE REG3, 2",          // Guarda REG3 (11.0) en memoria[2]
            "FADD REG4, REG1, REG2",  // REG4 = REG1 + REG2 = 7.5
            "INC REG1",               // REG1++ (incremento entero, no FP)
            "DEC REG2",               // REG2-- (decremento entero, no FP)
        };
        
        for (size_t i = 0; i < program_lines.size(); i++) {
            std::cout << "  [" << i << "] " << program_lines[i] << std::endl;
        }
        
        // 5. Usar el Loader para convertir el programa en instrucciones
        Loader loader;
        std::vector<Instruction> program = loader.parseProgram(program_lines);
        
        // 6. Instanciar el PE y cargar el programa y el backend de memoria (cache)
        PE pe(0);
        pe.attachMemory(&cacheMemPort);
        pe.loadProgram(program);
        
        // 7. Ejecutar el PE
        std::cout << "\n=== Ejecutando programa en PE ===" << std::endl;
        pe.runToCompletion();
        
        // 8. Mostrar estado de los registros del PE
        std::cout << "\n=== Estado final de registros PE ===" << std::endl;
        const uint64_t* regs = pe.regs();
        
        std::cout << std::left;
        for (int i = 0; i < 9; ++i) {
            std::cout << std::setw(6) << ("REG" + std::to_string(i) + ":");
            std::cout << " hex=0x" << std::hex << std::setw(16) << std::setfill('0') 
                      << regs[i] << std::dec << std::setfill(' ');
            std::cout << " | int=" << std::setw(20) << std::right << (int64_t)regs[i] << std::left;
            std::cout << " | float=" << std::setw(12) << std::fixed 
                      << std::setprecision(6) << uint64ToDouble(regs[i]);
            std::cout << std::endl;
        }
        
        // 9. Verificar memoria[2] después de la ejecución
        std::cout << "\n=== Verificando memoria después de ejecución ===" << std::endl;
        uint64_t result_mem2 = 0;
        cache.read(2 * sizeof(uint64_t), result_mem2);
        std::cout << "memoria[2] @ 0x10 = " << uint64ToDouble(result_mem2) 
                  << " (esperado: 11.0)" << std::endl;
        
        // 10. Mostrar estadísticas del PE
        std::cout << "\n=== Estadísticas del PE ===" << std::endl;
        std::cout << "Instrucciones ejecutadas: " << pe.getInstructionCount() << std::endl;
        std::cout << "Operaciones LOAD: " << pe.getLoadCount() << std::endl;
        std::cout << "Operaciones STORE: " << pe.getStoreCount() << std::endl;
        std::cout << "Ciclos simulados: " << pe.getCycleCount() << std::endl;
        
        // 11. Mostrar estadísticas y contenido del cache
        cache.printStats();
        cache.printCache();
        
        // 12. Verificar resultados esperados
        std::cout << "\n=== Verificación de resultados ===" << std::endl;
        double reg3_val = uint64ToDouble(regs[3]);
        double expected_mul = 11.0;
        double reg4_val = uint64ToDouble(regs[4]);
        double expected_add = 7.5;
        
        bool mul_ok = std::abs(reg3_val - expected_mul) < 0.0001;
        bool add_ok = std::abs(reg4_val - expected_add) < 0.0001;
        
        std::cout << "REG3 (FMUL 5.5*2.0): " << std::setprecision(2) << reg3_val 
                  << (mul_ok ? " ✓ CORRECTO" : " ✗ ERROR") << std::endl;
        std::cout << "REG4 (FADD 5.5+2.0): " << std::setprecision(2) << reg4_val 
                  << (add_ok ? " ✓ CORRECTO" : " ✗ ERROR") << std::endl;
        
        double mem2_val = uint64ToDouble(result_mem2);
        bool store_ok = std::abs(mem2_val - expected_mul) < 0.0001;
        std::cout << "MEM[2] (STORE REG3): " << std::setprecision(2) << mem2_val
                  << (store_ok ? " ✓ CORRECTO" : " ✗ ERROR") << std::endl;
        
        if (mul_ok && add_ok && store_ok) {
            std::cout << "\n✓✓✓ ¡Simulación completada exitosamente! ✓✓✓" << std::endl;
        } else {
            std::cout << "\n✗✗✗ ¡Advertencia: Resultados incorrectos! ✗✗✗" << std::endl;
        }
        
    } catch (const std::exception& ex) {
        std::cerr << "\n!!! Error durante la ejecución !!!" << std::endl;
        std::cerr << "Excepción: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}