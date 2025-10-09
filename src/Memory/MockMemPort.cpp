// src/Memory/MockMemPort.cpp
#include "MockMemPort.hpp"

MockMemPort::MockMemPort(size_t size, uint64_t latency_cycles)
    : mem(size, 0), latency(latency_cycles) {}

uint64_t MockMemPort::load(uint64_t addr) {
    if (addr >= mem.size())
        throw std::out_of_range("MockMemPort LOAD out of range");
    if (latency > 0) std::this_thread::sleep_for(std::chrono::nanoseconds(latency));
    return mem[addr];
}

void MockMemPort::store(uint64_t addr, uint64_t data) {
    if (addr >= mem.size())
        throw std::out_of_range("MockMemPort STORE out of range");
    if (latency > 0) std::this_thread::sleep_for(std::chrono::nanoseconds(latency));
    mem[addr] = data;
}
