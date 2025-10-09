// src/PE/PE.cpp
#include "PE.hpp"
#include <cstring>
#include <stdexcept>

PE::PE(int id)
: id_(id), running_(false), mem_(nullptr),
  instr_count_(0), load_count_(0), store_count_(0), cycle_count_(0) {
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

const uint64_t* PE::regs() const { return regs_; }

void PE::runToCompletion() {
    running_ = true;
    size_t pc = 0;
    while (running_ && pc < program_.size()) {
        executeInstruction(program_[pc], pc);
        ++instr_count_;
    }
    running_ = false;
}

void PE::threadMain() {
    size_t pc = 0;
    while (running_ && pc < program_.size()) {
        executeInstruction(program_[pc], pc);
        ++instr_count_;
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
            uint64_t addr = inst.imm;
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
