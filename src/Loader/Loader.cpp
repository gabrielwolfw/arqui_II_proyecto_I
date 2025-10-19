#include "Loader.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <regex>

std::string Loader::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> Loader::split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        std::string t = trim(item);
        if (!t.empty()) parts.push_back(t);
    }
    return parts;
}

int Loader::regIndex(const std::string &r) {
    std::string s = trim(r);
    for (auto &c: s) c = toupper(c);
    if (s.empty()) throw std::runtime_error("Registro vacío");

    // Solo aceptar formato Rn o REGn
    std::regex reg_regex("^R([0-9]+)$");
    std::regex reg_regex2("^REG([0-9]+)$");
    std::smatch match;
    if (std::regex_match(s, match, reg_regex)) {
        return std::stoi(match[1]);
    } else if (std::regex_match(s, match, reg_regex2)) {
        return std::stoi(match[1]);
    }

    // Si no es registro, devuelve -1 (para etiquetas u otros casos)
    return -1;
}

// ====================== Primera pasada: etiquetas ======================
void Loader::firstPass(const std::vector<std::string>& lines) {
    labels.clear();
    int inst_index = 0;

    for (int i = 0; i < (int)lines.size(); ++i) {
        std::string line = trim(lines[i]);
        if (line.empty() || line[0] == ';') continue;

        if (line.back() == ':') {
            std::string label = trim(line.substr(0, line.size() - 1));
            labels[label] = inst_index;
            continue; // No cuenta como instrucción
        }

        inst_index++;
    }
}

// ====================== Segunda pasada: instrucciones ======================
std::vector<Instruction> Loader::secondPass(const std::vector<std::string>& lines) {
    std::vector<Instruction> program;

    for (int i = 0; i < (int)lines.size(); ++i) {
        std::string line = trim(lines[i]);
        if (line.empty() || line[0] == ';') continue;

        // Saltar etiquetas
        if (line.back() == ':') continue;

        // Separar tokens
        std::vector<std::string> tokens;
        {
            std::stringstream ss(line);
            std::string word;
            while (ss >> word) {
                if (word.find(',') != std::string::npos) {
                    auto parts = split(word, ',');
                    tokens.insert(tokens.end(), parts.begin(), parts.end());
                } else {
                    tokens.push_back(trim(word));
                }
            }
        }

        if (tokens.empty()) continue;

        std::string opcode = tokens[0];
        for (auto &c : opcode) c = toupper(c);

        Instruction inst{};
        inst.op = OpCode::INVALID;
        inst.rd = inst.ra = inst.rb = -1;
        inst.imm = 0;

        if (opcode == "LOAD") {
            inst.op = OpCode::LOAD;
            // Sólo acepta registros válidos
            int rd = regIndex(tokens[1]);
            if (rd == -1) throw std::runtime_error("LOAD espera registro como destino: " + tokens[1]);
            inst.rd = rd;
            // Puede ser registro o inmediato
            int ra = regIndex(tokens[2]);
            if (ra != -1) {
                inst.ra = ra;
            } else {
                inst.imm = std::stoi(tokens[2]);
            }
        } 
        else if (opcode == "STORE") {
            inst.op = OpCode::STORE;
            int ra = regIndex(tokens[1]);
            if (ra == -1) throw std::runtime_error("STORE espera registro como fuente: " + tokens[1]);
            inst.ra = ra; // registro fuente del dato
            // Puede ser registro o inmediato
            int rb = regIndex(tokens[2]);
            if (rb != -1) {
                inst.rb = rb;
            } else {
                inst.imm = std::stoi(tokens[2]);
            }
        }
        else if (opcode == "FMUL") {
            inst.op = OpCode::FMUL;
            int rd = regIndex(tokens[1]);
            int ra = regIndex(tokens[2]);
            int rb = regIndex(tokens[3]);
            if (rd == -1 || ra == -1 || rb == -1) throw std::runtime_error("FMUL espera tres registros");
            inst.rd = rd;
            inst.ra = ra;
            inst.rb = rb;
        } 
        else if (opcode == "FADD") {
            inst.op = OpCode::FADD;
            int rd = regIndex(tokens[1]);
            int ra = regIndex(tokens[2]);
            int rb = regIndex(tokens[3]);
            if (rd == -1 || ra == -1 || rb == -1) throw std::runtime_error("FADD espera tres registros");
            inst.rd = rd;
            inst.ra = ra;
            inst.rb = rb;
        } 
        else if (opcode == "DIV") {
            inst.op = OpCode::DIV;
            int rd = regIndex(tokens[1]);
            int ra = regIndex(tokens[2]);
            if (rd == -1 || ra == -1) throw std::runtime_error("DIV espera registro destino y registro fuente");
            inst.rd = rd;
            inst.ra = ra;
            
            // El tercer operando puede ser registro o inmediato
            int rb = regIndex(tokens[3]);
            if (rb != -1) {
                inst.rb = rb;  // Es un registro
            } else {
                try {
                    inst.imm = std::stoi(tokens[3]);  // Es un inmediato
                    inst.rb = -1;  // Marcamos que usamos inmediato
                } catch (...) {
                    throw std::runtime_error("DIV espera registro o número como tercer operando");
                }
            }
        }
        else if (opcode == "MUL") {
            inst.op = OpCode::MUL;
            int rd = regIndex(tokens[1]);
            int ra = regIndex(tokens[2]);
            if (rd == -1 || ra == -1) throw std::runtime_error("MUL espera registro destino y registro fuente");
            inst.rd = rd;
            inst.ra = ra;
            
            // El tercer operando puede ser registro o inmediato
            int rb = regIndex(tokens[3]);
            if (rb != -1) {
                inst.rb = rb;  // Es un registro
            } else {
                try {
                    inst.imm = std::stoi(tokens[3]);  // Es un inmediato
                    inst.rb = -1;  // Marcamos que usamos inmediato
                } catch (...) {
                    throw std::runtime_error("MUL espera registro o número como tercer operando");
                }
            }
        }
        else if (opcode == "MOVE") {
            inst.op = OpCode::MOVE;
            int rd = regIndex(tokens[1]);
            if (rd == -1) throw std::runtime_error("MOVE espera registro como destino");
            inst.rd = rd;
            
            // El segundo operando puede ser registro o inmediato
            int ra = regIndex(tokens[2]);
            if (ra != -1) {
                inst.ra = ra;  // Es un registro
            } else {
                try {
                    inst.imm = std::stoi(tokens[2]);  // Es un inmediato
                    inst.ra = -1;  // Marcamos que usamos inmediato
                } catch (...) {
                    throw std::runtime_error("MOVE espera registro o número como segundo operando");
                }
            }
        }
        else if (opcode == "ADD") {
            inst.op = OpCode::ADD;
            int rd = regIndex(tokens[1]);
            int ra = regIndex(tokens[2]);
            if (rd == -1 || ra == -1) throw std::runtime_error("ADD espera registro destino y registro fuente");
            inst.rd = rd;
            inst.ra = ra;
            
            // El tercer operando puede ser registro o inmediato
            int rb = regIndex(tokens[3]);
            if (rb != -1) {
                inst.rb = rb;  // Es un registro
            } else {
                try {
                    inst.imm = std::stoi(tokens[3]);  // Es un inmediato
                    inst.rb = -1;  // Marcamos que usamos inmediato
                } catch (...) {
                    throw std::runtime_error("ADD espera registro o número como tercer operando");
                }
            }
        }
        else if (opcode == "CMP") {
            inst.op = OpCode::CMP;
            int ra = regIndex(tokens[1]);
            if (ra == -1) throw std::runtime_error("CMP espera registro como primer operando");
            inst.ra = ra;
            
            // El segundo operando puede ser registro o inmediato
            int rb = regIndex(tokens[2]);
            if (rb != -1) {
                inst.rb = rb;  // Es un registro
            } else {
                try {
                    inst.imm = std::stoi(tokens[2]);  // Es un inmediato
                    inst.rb = -1;  // Marcamos que usamos inmediato
                } catch (...) {
                    throw std::runtime_error("CMP espera registro o número como segundo operando");
                }
            }
        }
        else if (opcode == "JL") {
            inst.op = OpCode::JL;
            std::string target = trim(tokens[1]);
            // Si es etiqueta, busca en el diccionario
            if (labels.count(target)) {
                inst.imm = labels[target];
            } else {
                // Si no, intenta convertir a número
                try {
                    inst.imm = std::stoi(target);
                } catch (...) {
                    throw std::runtime_error("Etiqueta no encontrada para JL: '" + target + "'");
                }
            }
        }
        else if (opcode == "INC") {
            inst.op = OpCode::INC;
            int rd = regIndex(tokens[1]);
            if (rd == -1) throw std::runtime_error("INC espera registro como destino: " + tokens[1]);
            inst.rd = rd;
        } 
        else if (opcode == "DEC") {
            inst.op = OpCode::DEC;
            int rd = regIndex(tokens[1]);
            if (rd == -1) throw std::runtime_error("DEC espera registro como destino: " + tokens[1]);
            inst.rd = rd;
        } 
        else if (opcode == "JNZ") {
            inst.op = OpCode::JNZ;
            std::string target = trim(tokens[1]);
            // Si es etiqueta, busca en el diccionario
            if (labels.count(target)) {
                inst.imm = labels[target];
            } else {
                // Si no, intenta convertir a número
                try {
                    inst.imm = std::stoi(target);
                } catch (...) {
                    throw std::runtime_error("Etiqueta no encontrada para JNZ: '" + target + "'");
                }
            }
        }

        if (inst.op == OpCode::INVALID) {
            throw std::runtime_error("Instrucción inválida en línea: " + line);
        }

        program.push_back(inst);
    }

    return program;
}

std::vector<Instruction> Loader::parseProgram(const std::vector<std::string>& lines) {
    firstPass(lines);
    return secondPass(lines);
}

std::string Loader::parseInstructionToString(const Instruction& inst) {
    std::stringstream ss;
    switch (inst.op) {
        case OpCode::LOAD:
            if (inst.ra != -1)
                ss << "LOAD R" << inst.rd << ", R" << inst.ra;
            else
                ss << "LOAD R" << inst.rd << ", " << inst.imm;
            break;
        case OpCode::STORE:
            if (inst.rb != -1)
                ss << "STORE R" << inst.ra << ", R" << inst.rb;
            else
                ss << "STORE R" << inst.ra << ", " << inst.imm;
            break;
        case OpCode::FMUL:
            ss << "FMUL R" << inst.rd << ", R" << inst.ra << ", R" << inst.rb;
            break;
        case OpCode::FADD:
            ss << "FADD R" << inst.rd << ", R" << inst.ra << ", R" << inst.rb;
            break;
        case OpCode::DIV:
            if (inst.rb != -1) {
                ss << "DIV R" << inst.rd << ", R" << inst.ra << ", R" << inst.rb;
            } else {
                ss << "DIV R" << inst.rd << ", R" << inst.ra << ", " << inst.imm;
            }
            break;
        case OpCode::MUL:
            if (inst.rb != -1) {
                ss << "MUL R" << inst.rd << ", R" << inst.ra << ", R" << inst.rb;
            } else {
                ss << "MUL R" << inst.rd << ", R" << inst.ra << ", " << inst.imm;
            }
            break;
        case OpCode::MOVE:
            if (inst.ra != -1) {
                ss << "MOVE R" << inst.rd << ", R" << inst.ra;
            } else {
                ss << "MOVE R" << inst.rd << ", " << inst.imm;
            }
            break;
        case OpCode::ADD:
            if (inst.rb != -1) {
                ss << "ADD R" << inst.rd << ", R" << inst.ra << ", R" << inst.rb;
            } else {
                ss << "ADD R" << inst.rd << ", R" << inst.ra << ", " << inst.imm;
            }
            break;
        case OpCode::CMP:
            if (inst.rb != -1) {
                ss << "CMP R" << inst.ra << ", R" << inst.rb;
            } else {
                ss << "CMP R" << inst.ra << ", " << inst.imm;
            }
            break;
        case OpCode::JL:
            ss << "JL " << inst.imm;
            break;
        case OpCode::INC:
            ss << "INC R" << inst.rd;
            break;
        case OpCode::DEC:
            ss << "DEC R" << inst.rd;
            break;
        case OpCode::JNZ:
            ss << "JNZ " << inst.imm;
            break;
        default:
            ss << "INVALID";
            break;
    }
    return ss.str();
}