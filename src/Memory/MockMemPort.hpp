// src/Memory/MockMemPort.hpp
#pragma once
#include "IMemPort.hpp"
#include <vector>
#include <thread>
#include <chrono>
#include <stdexcept>

class MockMemPort : public IMemPort {
public:
    MockMemPort(size_t size, uint64_t latency_cycles = 0);

    uint64_t load(uint64_t addr) override;
    void store(uint64_t addr, uint64_t data) override;

    uint64_t* rawData() { return mem.data(); }
    size_t size() const { return mem.size(); }

private:
    std::vector<uint64_t> mem;
    uint64_t latency;
};
