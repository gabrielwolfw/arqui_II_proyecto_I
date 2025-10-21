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
    JNZ,    // JNZ label  (usa REG0 como contador implicito)
    DIV,    // DIV Rd, Ra, Rb    (división entera)
    MUL,    // MUL Rd, Ra, Rb    (multiplicación entera)
    MOVE,   // MOVE Rd, Ra       (mover entre registros)
    ADD,    // ADD Rd, Ra, Rb    (suma entera)
    CMP,    // CMP Ra, Rb        (compara Ra con Rb)
    JL,     // JL label          (salta si último CMP fue menor)
    JLE     // JLE label         (salta si último CMP fue menor o igual)
};

struct Instruction {
    OpCode op;
    int rd;   // registro destino
    int ra;   // registro A
    int rb;   // registro B
    int imm;  // inmediato (por ejemplo, dirección de memoria o salto)
};
