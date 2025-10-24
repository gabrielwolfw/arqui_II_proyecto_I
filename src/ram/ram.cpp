#include "ram.hpp"
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <sstream>

RAM::RAM(bool verbose) : verbose_(verbose) {
    // Initialize RAM with 512 positions of 64-bit words
    memory_.resize(RAM_SIZE, 0);
    
    if (verbose_) {
        std::cout << "RAM Initialized:\n"
                  << "- Capacity: " << RAM_SIZE << " positions × " << WORD_SIZE << " bits = "
                  << RAM_SIZE_BYTES << " bytes (" << RAM_SIZE_BYTES/1024 << " KB)\n"
                  << "- Addressing: 0x0000 to 0x" << std::hex << std::uppercase 
                  << MAX_ADDRESS << " (" << ADDRESS_BITS << " bits)\n"
                  << "- Word size: " << std::dec << WORD_SIZE << " bits (IEEE 754 double)\n";
    }
}

uint64_t RAM::read(uint32_t address) const {
    validateAddress(address);
    logOperation("READ", address, memory_[address]);
    return memory_[address];
}

void RAM::write(uint32_t address, uint64_t data) {
    validateAddress(address);
    memory_[address] = data;
    logOperation("WRITE", address, data);
}

bool RAM::isValidAddress(uint32_t address) const {
    return address <= MAX_ADDRESS;
}

void RAM::validateAddress(uint32_t address) const {
    if (!isValidAddress(address)) {
        throw std::out_of_range("Invalid memory address: 0x" + 
                               std::to_string(address) + 
                               ". Valid range is 0x0000 to 0x" + 
                               std::to_string(MAX_ADDRESS));
    }
}

void RAM::logOperation(const std::string& op, uint32_t address, uint64_t data) const {
    if (verbose_) {
        std::cout << op << " @ 0x" << std::hex << std::uppercase << std::setfill('0') 
                  << std::setw(4) << address << ": 0x" << std::setw(16) 
                  << data << std::dec << std::endl;
    }
}

double RAM::readAsDouble(uint32_t address) const {
    uint64_t raw_bits = read(address);   // Lee los 64 bits crudos de la RAM
    double value;
    std::memcpy(&value, &raw_bits, sizeof(double)); // Copia los bits al double
    return value;
}

void RAM::writeAsDouble(uint32_t address, double value) {
    uint64_t raw_bits;
    std::memcpy(&raw_bits, &value, sizeof(double)); // Copia los bits del double
    write(address, raw_bits);  // Escribe los 64 bits crudos en la RAM
}

// ============================================================
// VECTOR LOADING UTILITIES
// ============================================================

std::vector<double> RAM::loadVectorFromFile(const std::string& filepath) {
    std::vector<double> data;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + filepath);
    }
    
    double value;
    std::string line;
    int line_number = 0;
    
    while (std::getline(file, line)) {
        line_number++;
        
        // Ignorar líneas vacías y comentarios
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Intentar parsear el valor
        std::istringstream iss(line);
        if (iss >> value) {
            data.push_back(value);
        } else {
            std::cerr << "Warning: No se pudo parsear línea " << line_number 
                     << " en " << filepath << ": '" << line << "'" << std::endl;
        }
    }
    
    file.close();
    
    if (verbose_) {
        std::cout << "[RAM] Cargados " << data.size() 
                  << " valores desde " << filepath << std::endl;
    }
    
    return data;
}

void RAM::loadVectorsToMemory(const std::string& vector_a_file, 
                              const std::string& vector_b_file) {
    if (verbose_) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  CARGANDO VECTORES DESDE ARCHIVOS" << std::endl;
        std::cout << "========================================\n" << std::endl;
    }
    
    // Cargar vectores desde archivos
    auto vector_a = loadVectorFromFile(vector_a_file);
    auto vector_b = loadVectorFromFile(vector_b_file);
    
    // Delegar al otro método
    loadVectorsToMemory(vector_a, vector_b);
}

void RAM::loadVectorsToMemory(const std::vector<double>& vector_a,
                              const std::vector<double>& vector_b) {
    // Validar tamaños
    if (vector_a.size() != vector_b.size()) {
        throw std::runtime_error("Los vectores deben tener el mismo tamaño. "
                                "Vector A: " + std::to_string(vector_a.size()) + 
                                ", Vector B: " + std::to_string(vector_b.size()));
    }
    
    size_t vector_length = vector_a.size();
    
    // Validar que caben en memoria
    // Necesitamos: 1 (longitud) + N + N + N = 3N + 1 posiciones
    size_t required_memory = 3 * vector_length + 1;
    if (required_memory > RAM_SIZE) {
        throw std::runtime_error("Los vectores son demasiado grandes para la RAM. "
                                "Requerido: " + std::to_string(required_memory) + 
                                " posiciones, Disponible: " + std::to_string(RAM_SIZE));
    }
    
    if (verbose_) {
        std::cout << "=== Estructura de Memoria ===" << std::endl;
        std::cout << "Longitud de vectores: " << vector_length << std::endl;
        std::cout << "Memoria requerida: " << required_memory << " / " 
                  << RAM_SIZE << " posiciones" << std::endl;
    }
    
    // Guardar longitud en mem[0]
    write(0, static_cast<uint64_t>(vector_length));
    if (verbose_) {
        std::cout << "\nmem[0] = " << vector_length << " (longitud de vectores)" << std::endl;
    }
    
    // Guardar vector A en mem[1..N]
    if (verbose_) {
        std::cout << "\n=== Vector A ===" << std::endl;
    }
    for (size_t i = 0; i < vector_length; i++) {
        writeAsDouble(i + 1, vector_a[i]);
        if (verbose_ && i < 5) {  // Mostrar solo los primeros 5
            std::cout << "  mem[" << std::setw(3) << (i + 1) << "] = " 
                     << std::fixed << std::setprecision(2) << vector_a[i] << std::endl;
        }
    }
    if (verbose_ && vector_length > 5) {
        std::cout << "  ... (" << (vector_length - 5) << " elementos más)" << std::endl;
    }
    
    // Guardar vector B en mem[N+1..2N]
    if (verbose_) {
        std::cout << "\n=== Vector B ===" << std::endl;
    }
    for (size_t i = 0; i < vector_length; i++) {
        writeAsDouble(vector_length + i + 1, vector_b[i]);
        if (verbose_ && i < 5) {  // Mostrar solo los primeros 5
            std::cout << "  mem[" << std::setw(3) << (vector_length + i + 1) 
                     << "] = " << std::fixed << std::setprecision(2) 
                     << vector_b[i] << std::endl;
        }
    }
    if (verbose_ && vector_length > 5) {
        std::cout << "  ... (" << (vector_length - 5) << " elementos más)" << std::endl;
    }
    
    // Inicializar vector resultado en mem[2N+1..3N] con ceros
    if (verbose_) {
        std::cout << "\n=== Vector Resultado (inicializado en 0) ===" << std::endl;
    }
    for (size_t i = 0; i < vector_length; i++) {
        writeAsDouble(2 * vector_length + i + 1, 0.0);
        if (verbose_ && i < 5) {  // Mostrar solo los primeros 5
            std::cout << "  mem[" << std::setw(3) << (2 * vector_length + i + 1) 
                     << "] = 0.0" << std::endl;
        }
    }
    if (verbose_ && vector_length > 5) {
        std::cout << "  ... (" << (vector_length - 5) << " elementos más)" << std::endl;
    }
    
    if (verbose_) {
        printMemoryMap(vector_length);
    }
}

void RAM::printMemoryMap(size_t vector_length) const {
    std::cout << "\n=== Mapa de Memoria ===" << std::endl;
    std::cout << "  mem[0]                : Longitud (N = " << vector_length << ")" << std::endl;
    std::cout << "  mem[1 .. " << std::setw(3) << vector_length << "]        : Vector A" << std::endl;
    std::cout << "  mem[" << std::setw(3) << (vector_length + 1) << " .. " 
              << std::setw(3) << (2 * vector_length) << "]      : Vector B" << std::endl;
    std::cout << "  mem[" << std::setw(3) << (2 * vector_length + 1) << " .. " 
              << std::setw(3) << (3 * vector_length) << "]      : Vector Resultado" << std::endl;
    std::cout << "  mem[" << std::setw(3) << (3 * vector_length + 1) << " .. " 
              << std::setw(3) << MAX_ADDRESS << "] : Libre" << std::endl;
}

void RAM::printVectorData(size_t start_index, size_t length, 
                         const std::string& vector_name) const {
    std::cout << "\n=== " << vector_name << " ===" << std::endl;
    
    for (size_t i = 0; i < length; i++) {
        uint32_t addr = start_index + i;
        double value = readAsDouble(addr);
        std::cout << "  " << vector_name << "[" << std::setw(2) << i << "] = "
                  << "mem[" << std::setw(3) << addr << "] = "
                  << std::fixed << std::setprecision(2) << value << std::endl;
    }
}

void RAM::verifyLoadedData() const {
    uint64_t N = read(0);
    
    if (N == 0) {
        std::cout << "\n No hay datos cargados en memoria (N = 0)" << std::endl;
        return;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  VERIFICACIÓN DE DATOS CARGADOS" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::cout << "\nLongitud de vectores (mem[0]): " << N << std::endl;
    
    // Mostrar primeros elementos de cada vector
    size_t preview_size = std::min((uint64_t)5, N);
    
    std::cout << "\nPrimeros " << preview_size << " elementos:" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    for (size_t i = 0; i < preview_size; i++) {
        double val_a = readAsDouble(i + 1);
        double val_b = readAsDouble(N + i + 1);
        double val_c = readAsDouble(2 * N + i + 1);
        
        std::cout << "  [" << i << "] A = " << std::setw(8) << val_a
                  << " | B = " << std::setw(8) << val_b
                  << " | C = " << std::setw(8) << val_c << std::endl;
    }
    
    if (N > preview_size) {
        std::cout << "  ... (" << (N - preview_size) << " elementos más)\n";
    }
    
    // Estadísticas
    double sum_a = 0.0, sum_b = 0.0;
    for (size_t i = 0; i < N; i++) {
        sum_a += readAsDouble(i + 1);
        sum_b += readAsDouble(N + i + 1);
    }
    
    std::cout << "\nEstadísticas:" << std::endl;
    std::cout << "  Promedio Vector A: " << (sum_a / N) << std::endl;
    std::cout << "  Promedio Vector B: " << (sum_b / N) << std::endl;
    std::cout << "  Memoria utilizada: " << (3 * N + 1) << " / " << RAM_SIZE 
              << " posiciones (" << std::setprecision(1) 
              << (100.0 * (3 * N + 1) / RAM_SIZE) << "%)" << std::endl;
}