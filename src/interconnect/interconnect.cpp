#include "interconnect.hpp"
#include <iostream>

Interconnect::Interconnect(std::shared_ptr<RAM> ram, bool verbose)
    : ram_(ram)
    , pe_queues_(4)  // Support for 4 Processing Elements
    , current_pe_(0)
    , verbose_(verbose) {
}

void Interconnect::addRequest(const BusTransaction& transaction) {
    if (transaction.pe_id >= pe_queues_.size()) {
        throw std::runtime_error("Invalid PE ID");
    }
    pe_queues_[transaction.pe_id].push(transaction);
    
    if (verbose_) {
        std::cout << "Added transaction: " << transaction.toString() << std::endl;
    }
}

size_t Interconnect::getNextPE() {
    size_t start_pe = current_pe_;
    do {
        if (!pe_queues_[current_pe_].empty()) {
            return current_pe_;
        }
        current_pe_ = (current_pe_ + 1) % pe_queues_.size();
    } while (current_pe_ != start_pe);
    
    return pe_queues_.size();  // Invalid PE if none found
}

bool Interconnect::processNextTransaction() {
    size_t next_pe = getNextPE();
    if (next_pe >= pe_queues_.size()) {
        return false;  // No transactions to process
    }
    
    BusTransaction& transaction = pe_queues_[next_pe].front();
    
    if (verbose_) {
        std::cout << "Processing transaction: " << transaction.toString() << std::endl;
    }
    
    // Handle the transaction based on type
    switch (transaction.type) {
        case BusTransactionType::BusRd:
            transaction.data = ram_->read(transaction.address);
            break;
            
        case BusTransactionType::BusRdX:
            transaction.data = ram_->read(transaction.address);
            break;
            
        case BusTransactionType::BusWB:
            ram_->write(transaction.address, transaction.data);
            break;
            
        case BusTransactionType::BusUpgr:
            // No memory operation needed, just state transition
            break;
    }
    
    processed_transactions_.push_back(transaction);
    pe_queues_[next_pe].pop();
    current_pe_ = (next_pe + 1) % pe_queues_.size();
    
    return true;
}

bool Interconnect::hasPendingTransactions() const {
    for (const auto& queue : pe_queues_) {
        if (!queue.empty()) {
            return true;
        }
    }
    return false;
}

const std::vector<BusTransaction>& Interconnect::getProcessedTransactions() const {
    return processed_transactions_;
}