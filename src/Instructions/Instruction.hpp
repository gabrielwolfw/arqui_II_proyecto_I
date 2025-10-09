// Instruction.hpp
#pragma once
#include <string>
#include <vector>

enum class OpCode {
    INVALID = 0,
    NOP,
    LOAD,   // LOAD REG, addr
    STORE,  // STORE REG, addr
    FMUL,   // FMUL Rd, Ra, Rb
    FADD,   // FADD Rd, Ra, Rb
    INC,    // INC REG
    DEC,    // DEC REG
    JNZ     // JNZ label  (usa REG0 como contador implicito)
};

struct Instruction {
    OpCode op;
    int rd;   // registro destino
    int ra;   // registro A
    int rb;   // registro B
    int imm;  // inmediato (por ejemplo, dirección de memoria o salto)
};
