#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <stdexcept>
#include <string>

// Lee un archivo de vector y devuelve los valores
std::vector<double> loadVectorFromFile(const std::string& filename) {
    std::vector<double> values;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + filename);
    }

    double value;
    while (file >> value) {
        values.push_back(value);
    }

    return values;
}

// Calcula la suma parcial para un rango específico
double calculatePartialSum(const std::vector<double>& A, const std::vector<double>& B,
                         size_t start, size_t count) {
    double sum = 0.0;
    for (size_t i = 0; i < count && (start + i) < A.size(); ++i) {
        sum += A[start + i] * B[start + i];
    }
    return sum;
}

int main() {
    try {
        // Cargar vectores
        auto vector_a = loadVectorFromFile("../../src/vectores/vector_a.txt");
        auto vector_b = loadVectorFromFile("../../src/vectores/vector_b.txt");

        if (vector_a.size() != vector_b.size()) {
            throw std::runtime_error("Los vectores tienen diferentes tamaños");
        }

        size_t N = vector_a.size();
        size_t elements_per_pe = N / 4;

        std::cout << "Tamaño de los vectores: " << N << std::endl;
        std::cout << "Elementos por PE: " << elements_per_pe << std::endl;

        // Mostrar vectores
        std::cout << "\nVector A: ";
        for (double val : vector_a) {
            std::cout << std::fixed << std::setprecision(1) << val << " ";
        }
        
        std::cout << "\nVector B: ";
        for (double val : vector_b) {
            std::cout << std::fixed << std::setprecision(1) << val << " ";
        }
        std::cout << "\n" << std::endl;

        // Calcular sumas parciales por PE
        std::vector<double> partial_sums;
        double total_sum = 0.0;

        for (size_t pe = 0; pe < 4; ++pe) {
            size_t start = pe * elements_per_pe;
            double partial = calculatePartialSum(vector_a, vector_b, start, elements_per_pe);
            partial_sums.push_back(partial);
            total_sum += partial;

            std::cout << "PE" << pe << " (elementos " << start << "-" 
                     << (start + elements_per_pe - 1) << "):" << std::endl;
            std::cout << "  Valores A: ";
            for (size_t i = 0; i < elements_per_pe; ++i) {
                std::cout << std::fixed << std::setprecision(1) 
                         << vector_a[start + i] << " ";
            }
            std::cout << "\n  Valores B: ";
            for (size_t i = 0; i < elements_per_pe; ++i) {
                std::cout << std::fixed << std::setprecision(1) 
                         << vector_b[start + i] << " ";
            }
            std::cout << "\n  Suma parcial = " << std::fixed << std::setprecision(2) 
                     << partial << "\n" << std::endl;
        }

        // Mostrar resultado final
        std::cout << "=== Resumen ===" << std::endl;
        for (size_t i = 0; i < partial_sums.size(); ++i) {
            std::cout << "Suma parcial PE" << i << " (mem[" << (24 + i) 
                     << "]) = " << std::fixed << std::setprecision(2) 
                     << partial_sums[i] << std::endl;
        }
        std::cout << "\nProducto punto total = " << std::fixed << std::setprecision(2) 
                  << total_sum << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}