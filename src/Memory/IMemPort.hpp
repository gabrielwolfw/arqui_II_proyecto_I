// src/Memory/IMemPort.hpp
#pragma once
#include <cstdint>

class IMemPort {
public:
    virtual uint64_t load(uint64_t addr) = 0;
    virtual void store(uint64_t addr, uint64_t data) = 0;
    virtual ~IMemPort() = default;
};
