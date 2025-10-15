#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <queue>
#include <vector>
#include <memory>
#include "../bus/bus.hpp"
#include "../ram/ram.hpp"

class Interconnect {
public:
    Interconnect(std::shared_ptr<RAM> ram, bool verbose = false);
    
    // Add a new transaction request to the queue
    void addRequest(const BusTransaction& transaction);
    
    // Process one transaction using round-robin arbitration
    bool processNextTransaction();
    
    // Get processed transactions for verification
    const std::vector<BusTransaction>& getProcessedTransactions() const;
    
    // Check if there are pending transactions
    bool hasPendingTransactions() const;

private:
    std::shared_ptr<RAM> ram_;
    std::vector<std::queue<BusTransaction>> pe_queues_;  // Queue per PE
    std::vector<BusTransaction> processed_transactions_; // History
    size_t current_pe_;  // For round-robin
    bool verbose_;
    
    // Find next PE with pending transaction
    size_t getNextPE();
};

#endif // INTERCONNECT_HPP