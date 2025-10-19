#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "../Instructions/Instruction.hpp"

class Loader {
public:
    Loader() = default;

    // Convierte líneas de texto en un vector de instrucciones ejecutables
    std::vector<Instruction> parseProgram(const std::vector<std::string>& lines);
    std::string parseInstructionToString(const Instruction& inst);

private:
    std::unordered_map<std::string, int> labels;

    void firstPass(const std::vector<std::string>& lines);
    std::vector<Instruction> secondPass(const std::vector<std::string>& lines);

    static std::string trim(const std::string& s);
    static std::vector<std::string> split(const std::string& s, char delim);

    // Devuelve el índice del registro si es válido, o -1 si no lo es (por ejemplo, si es una etiqueta)
    static int regIndex(const std::string &r);
};