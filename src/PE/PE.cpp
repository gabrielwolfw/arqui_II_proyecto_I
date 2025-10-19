// src/PE/PE.cpp
#include <iostream>
#include "PE.hpp"
#include <cstring>
#include <stdexcept>

PE::PE(int id)
: id_(id), running_(false), mem_(nullptr),
  instr_count_(0), load_count_(0), store_count_(0), cycle_count_(0), int_instr_count_(0) {
    std::memset(regs_, 0, sizeof(regs_));
}

void PE::loadProgram(const std::vector<Instruction>& prog) { program_ = prog; }
void PE::attachMemory(IMemPort* mem) { mem_ = mem; }

void PE::start() {
    if (running_) return;
    running_ = true;
    thr_ = std::thread(&PE::threadMain, this);
}
void PE::join() { if (thr_.joinable()) thr_.join(); }
void PE::stop() { running_ = false; }

uint64_t PE::getInstructionCount() const { return instr_count_; }
uint64_t PE::getLoadCount() const { return load_count_; }
uint64_t PE::getStoreCount() const { return store_count_; }
uint64_t PE::getCycleCount() const { return cycle_count_; }
int PE::getIntInstructionCount() const { return int_instr_count_; }

const uint64_t* PE::regs() const { return regs_; }

void PE::runToCompletion() {
    running_ = true;
    size_t pc = 0;
    uint64_t max_instructions = 1000000;  // Límite de seguridad
    uint64_t executed = 0;
    
    while (running_ && pc < program_.size() && executed < max_instructions) {
        std::cout << "PE" << id_ << " ejecutando instrucción " << executed 
                  << " en PC=" << pc << std::endl;
        
        executeInstruction(program_[pc], pc);
        ++instr_count_;
        ++int_instr_count_;
        ++executed;
        
        if (executed == max_instructions) {
            std::cerr << "¡Alerta! PE" << id_ << " alcanzó el límite de instrucciones." << std::endl;
        }
    }
    running_ = false;
    
    std::cout << "PE" << id_ << " terminó después de " << executed 
              << " instrucciones." << std::endl;
}

void PE::threadMain() {
    size_t pc = 0;
    while (running_ && pc < program_.size()) {
        executeInstruction(program_[pc], pc);
        ++instr_count_;
        ++int_instr_count_;
    }
    running_ = false;
}

static double uint64ToDouble(uint64_t x) {
    double d;
    std::memcpy(&d, &x, sizeof(double));
    return d;
}
static uint64_t doubleToUint64(double d) {
    uint64_t x;
    std::memcpy(&x, &d, sizeof(double));
    return x;
}

void PE::executeInstruction(const Instruction& inst, size_t &pc) {
    switch (inst.op) {
        case OpCode::LOAD: {
            uint64_t addr = (inst.ra >= 0) ? regs_[inst.ra] : inst.imm;
            regs_[inst.rd] = mem_->load(addr);
            load_count_++;
            cycle_count_ += 1;
            ++pc;
            break;
        }
        case OpCode::STORE: {
            uint64_t addr = (inst.rb >= 0) ? regs_[inst.rb] : inst.imm;
            mem_->store(addr, regs_[inst.ra]);
            store_count_++;
            cycle_count_ += 1;
            ++pc;
            break;
        }
        case OpCode::FMUL: {
            double a = uint64ToDouble(regs_[inst.ra]);
            double b = uint64ToDouble(regs_[inst.rb]);
            regs_[inst.rd] = doubleToUint64(a * b);
            cycle_count_ += 1;
            ++pc;
            break;
        }
        case OpCode::FADD: {
            double a = uint64ToDouble(regs_[inst.ra]);
            double b = uint64ToDouble(regs_[inst.rb]);
            regs_[inst.rd] = doubleToUint64(a + b);
            cycle_count_ += 1;
            ++pc;
            break;
        }
        case OpCode::DIV: {
            // División entera con manejo de signos
            int64_t a = static_cast<int64_t>(regs_[inst.ra]);
            int64_t b = (inst.rb >= 0) ? static_cast<int64_t>(regs_[inst.rb]) : inst.imm;
            if (b == 0) {
                throw std::runtime_error("División por cero");
            }
            regs_[inst.rd] = static_cast<uint64_t>(a / b);
            std::cout << "DIV: " << a << " / " << b << " = " << (a / b) << std::endl;
            cycle_count_ += 10;  // División es más costosa
            ++pc;
            break;
        }
        case OpCode::MUL: {
            // Multiplicación entera
            uint64_t a = regs_[inst.ra];
            uint64_t b = (inst.rb >= 0) ? regs_[inst.rb] : inst.imm;
            regs_[inst.rd] = a * b;
            cycle_count_ += 5;  // Multiplicación es costosa
            ++pc;
            break;
        }
        case OpCode::MOVE: {
            // Mover valor entre registros o cargar inmediato
            regs_[inst.rd] = (inst.ra >= 0) ? regs_[inst.ra] : inst.imm;
            cycle_count_ += 1;
            ++pc;
            break;
        }
        case OpCode::ADD: {
            // Suma entera
            uint64_t a = regs_[inst.ra];
            uint64_t b = (inst.rb >= 0) ? regs_[inst.rb] : inst.imm;
            regs_[inst.rd] = a + b;
            cycle_count_ += 1;
            ++pc;
            break;
        }
        case OpCode::CMP: {
            // Comparación de números sin signo
            uint64_t a = regs_[inst.ra];
            uint64_t b = (inst.rb >= 0) ? regs_[inst.rb] : inst.imm;
            
            // Para comparar correctamente los valores como enteros con signo
            int64_t signed_a = static_cast<int64_t>(a);
            int64_t signed_b = static_cast<int64_t>(b);
            
            // Guardamos el resultado en reg[0] para las instrucciones de salto
            if (signed_a < signed_b) regs_[0] = 1;      // Less
            else if (signed_a == signed_b) regs_[0] = 0; // Equal
            else regs_[0] = 2;             // Greater
            
            // Debug
            std::cout << "CMP: " << signed_a << " vs " << signed_b 
                      << " = " << regs_[0] << std::endl;
            
            cycle_count_ += 1;
            ++pc;
            break;
        }
        case OpCode::JL: {
            // Salta si la última comparación fue "menor que"
            if (regs_[0] == 1) {  // Si el último CMP resultó en "menor que"
                std::cout << "JL: Saltando a " << inst.imm << std::endl;
                pc = inst.imm;
            } else {
                std::cout << "JL: No salta, continúa" << std::endl;
                ++pc;
            }
            cycle_count_ += 1;
            break;
        }
        case OpCode::INC: {
            regs_[inst.rd] = (uint64_t)((int64_t)regs_[inst.rd] + 1);
            cycle_count_ += 1;
            ++pc;
            break;
        }
        case OpCode::DEC: {
            regs_[inst.rd] = (uint64_t)((int64_t)regs_[inst.rd] - 1);
            cycle_count_ += 1;
            ++pc;
            break;
        }
        case OpCode::JNZ: {
            if ((int64_t)regs_[0] != 0)
                pc = inst.imm;
            else
                ++pc;
            cycle_count_ += 1;
            break;
        }
        default:
            ++pc;
            break;
    }
}
