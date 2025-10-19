// src/PE/PE.hpp
#pragma once
#include "Instruction.hpp"
#include "../Memory/IMemPort.hpp"
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>

class PE {
public:
    PE(int id);

    void loadProgram(const std::vector<Instruction>& prog);
    void attachMemory(IMemPort* mem);
    void start();
    void join();
    void stop();
    void runToCompletion();

    uint64_t getInstructionCount() const;
    uint64_t getLoadCount() const;
    uint64_t getStoreCount() const;
    uint64_t getCycleCount() const;
    int getIntInstructionCount() const;

    const uint64_t* regs() const;
    int getId() const { return id_; }

private:
    void threadMain();
    void executeInstruction(const Instruction& inst, size_t &pc);

    int id_;
    std::thread thr_;
    std::atomic<bool> running_;
    std::vector<Instruction> program_;
    IMemPort* mem_;
    uint64_t regs_[9];

    uint64_t instr_count_;
    uint64_t load_count_;
    uint64_t store_count_;
    uint64_t cycle_count_;
    int int_instr_count_;
};
