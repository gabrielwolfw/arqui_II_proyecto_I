#include "Loader.hpp"
#include "PE.hpp"
#include "MockMemPort.hpp"
#include <iostream>
#include <cstring>
#include <cmath>
#include <fstream>
#include <vector>

std::vector<std::string> loadProgramFromFile(const std::string& path);

int main() {
    try {
        // ==== Primera prueba ====
        MockMemPort mem1(16);
        double a = 25.0, b = 5.0;
        std::memcpy(mem1.rawData(), &a, sizeof(double));
        std::memcpy(mem1.rawData() + 1, &b, sizeof(double));

        Loader loader;
        auto prog1 = loader.parseProgram(loadProgramFromFile("Programs/program1.txt"));

        PE pe1(0);
        pe1.attachMemory(&mem1);
        pe1.loadProgram(prog1);
        pe1.runToCompletion();

        double res;
        std::memcpy(&res, mem1.rawData() + 2, sizeof(double));
        std::cout << "Result mem[2] = " << res << "\n";
        std::cout << "LOAD count: " << pe1.getLoadCount()
                  << " STORE count: " << pe1.getStoreCount()
                  << " CYCLES: " << pe1.getCycleCount() << "\n";

        // ==== Segunda prueba ====
        MockMemPort mem2(16);
        mem2.store(2, 5); // iteraciones
        mem2.store(3, 0); // acumulador inicial

        auto prog2 = loader.parseProgram(loadProgramFromFile("Programs/program2.txt"));
        PE pe2(1);
        pe2.attachMemory(&mem2);
        pe2.loadProgram(prog2);
        pe2.runToCompletion();

        uint64_t acc = mem2.load(1);
        std::cout << "Accumulator in mem[1] = " << acc << "\n";
        std::cout << "LOAD count: " << pe2.getLoadCount()
                  << " STORE count: " << pe2.getStoreCount()
                  << " CYCLES: " << pe2.getCycleCount() << "\n";

    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}

std::vector<std::string> loadProgramFromFile(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + path);
    }
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}
