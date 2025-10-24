#pragma once
#include <atomic>
#include <iostream>
#include <condition_variable>
#include <mutex>

class Clock {
public:
    static Clock& getInstance() {
        static Clock instance;
        return instance;
    }

    void tick() {
        if (stepping_mode_) {
            std::unique_lock<std::mutex> lock(mutex_);
            cycle_count_++;
            std::cout << "\n=== Ciclo de reloj: " << cycle_count_ << " ===\n" << std::endl;
            cv_.wait(lock, [this] { return continue_execution_; });
            continue_execution_ = false;
        } else {
            cycle_count_++;
        }
    }

    void step() {
        std::lock_guard<std::mutex> lock(mutex_);
        continue_execution_ = true;
        cv_.notify_one();
    }

    void setSteppingMode(bool enabled) {
        stepping_mode_ = enabled;
    }

    uint64_t getCycleCount() const {
        return cycle_count_;
    }

private:
    Clock() : cycle_count_(0), stepping_mode_(false), continue_execution_(false) {}
    
    std::atomic<uint64_t> cycle_count_;
    std::atomic<bool> stepping_mode_;
    bool continue_execution_;
    std::mutex mutex_;
    std::condition_variable cv_;
};