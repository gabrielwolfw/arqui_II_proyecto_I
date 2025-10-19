#ifndef RAM_HPP
#define RAM_HPP

#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include <string>

class RAM {
public:
    // Constants
    static constexpr size_t RAM_SIZE = 512;      // 512 palabras
    static constexpr size_t WORD_SIZE = 64;      // 64 bits por palabra
    static constexpr size_t RAM_SIZE_BYTES = RAM_SIZE * (WORD_SIZE / 8);
    static constexpr uint32_t MAX_ADDRESS = RAM_SIZE - 1;
    static constexpr size_t ADDRESS_BITS = 9;    // log2(512)

    // Constructor
    explicit RAM(bool verbose = false);

    // Basic memory operations
    uint64_t read(uint32_t address) const;
    void write(uint32_t address, uint64_t data);
    
    // Double precision operations
    double readAsDouble(uint32_t address) const;
    void writeAsDouble(uint32_t address, double value);

    // Address validation
    bool isValidAddress(uint32_t address) const;

    // Vector loading utilities
    std::vector<double> loadVectorFromFile(const std::string& filepath);
    void loadVectorsToMemory(const std::string& vector_a_file, 
                            const std::string& vector_b_file);
    void loadVectorsToMemory(const std::vector<double>& vector_a,
                            const std::vector<double>& vector_b);
    
    // Memory inspection utilities
    void printMemoryMap(size_t vector_length) const;
    void printVectorData(size_t start_index, size_t length, 
                        const std::string& vector_name) const;
    void verifyLoadedData() const;

private:
    std::vector<uint64_t> memory_;
    bool verbose_;

    void validateAddress(uint32_t address) const;
    void logOperation(const std::string& op, uint32_t address, uint64_t data) const;
};

#endif // RAM_HPP