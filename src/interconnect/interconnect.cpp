#include "interconnect.hpp"
#include <iostream>

Interconnect::Interconnect(std::shared_ptr<RAM> ram, bool verbose)
    : ram_(ram)
    , pe_queues_(4)  // Support for 4 Processing Elements
    , current_pe_(0)
    , verbose_(verbose) {
}

void Interconnect::registerCache(Cache* cache) {
    if (cache) {
        caches_.push_back(cache);
        if (verbose_) {
            std::cout << "[Interconnect] Registered new cache, total: " 
                      << caches_.size() << std::endl;
        }
    }
}

void Interconnect::addRequest(const BusTransaction& transaction) {
    if (transaction.pe_id >= pe_queues_.size()) {
        throw std::runtime_error("Invalid PE ID");
    }
    pe_queues_[transaction.pe_id].push(transaction);
    
    if (verbose_) {
        std::cout << "[Interconnect] Broadcasting message from PE" 
                  << transaction.pe_id << " for addr=0x" 
                  << std::hex << transaction.address << std::dec << std::endl;
    }
    
    // Notificar a todas las cachés excepto al emisor
    for (Cache* cache : caches_) {
        if (cache) {
            // transaction.type es BusTransactionType
            switch (transaction.type) {
                case BusTransactionType::BusRd:
                    // transaction.address es un índice de palabra/bloque; convertir a bytes
                    cache->handleBusRead(static_cast<uint64_t>(transaction.address) * sizeof(uint64_t));
                    break;
                case BusTransactionType::BusRdX:
                    cache->handleBusReadX(static_cast<uint64_t>(transaction.address) * sizeof(uint64_t));
                    break;
                case BusTransactionType::BusUpgr:
                    cache->invalidateLine(static_cast<uint64_t>(transaction.address) * sizeof(uint64_t));
                    break;
                default:
                    // BusWB y otros no requieren notificación de invalidación/lectura
                    break;
            }
        }
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
    
    // Handle transaction y notificar a las cachés cuando sea necesario
    switch (transaction.type) {
        case BusTransactionType::BusRd:
            transaction.data = ram_->read(transaction.address);
            // Notificar BUS_READ a otras cachés
            notifyCaches(transaction.address * sizeof(uint64_t), 
                        transaction.pe_id, BusEvent::BUS_READ);
            break;
            
        case BusTransactionType::BusRdX:
            transaction.data = ram_->read(transaction.address);
            // Notificar BUS_READX a otras cachés
            notifyCaches(transaction.address * sizeof(uint64_t), 
                        transaction.pe_id, BusEvent::BUS_READX);
            break;
            
        case BusTransactionType::BusWB:
            ram_->write(transaction.address, transaction.data);
            break;
            
        case BusTransactionType::BusUpgr:
            // Notificar BUS_UPGRADE a otras cachés
            notifyCaches(transaction.address * sizeof(uint64_t), 
                        transaction.pe_id, BusEvent::BUS_UPGRADE);
            break;
    }
    
    processed_transactions_.push_back(transaction);
    pe_queues_[next_pe].pop();
    current_pe_ = (next_pe + 1) % pe_queues_.size();
    
    return true;
}

void Interconnect::notifyCaches(uint64_t address, int sender_pe_id, BusEvent event) {
    BusMessage msg{address, event, sender_pe_id};
    broadcastBusMessage(msg);
}

void Interconnect::broadcastBusMessage(const BusMessage& msg) {
    if (verbose_) {
        std::cout << "[Interconnect] Broadcasting message from PE" 
                  << msg.sender_pe_id << " for addr=0x" 
                  << std::hex << msg.address << std::dec << std::endl;
    }
    
    // Notificar a todas las cachés excepto al emisor
    for (Cache* cache : caches_) {
        if (cache) {
            switch (msg.event) {
                case BusEvent::BUS_READ:
                    cache->handleBusRead(msg.address);
                    break;
                case BusEvent::BUS_READX:
                    cache->handleBusReadX(msg.address);
                    break;
                case BusEvent::BUS_UPGRADE:
                    cache->invalidateLine(msg.address);
                    break;
                default:
                    break;
            }
        }
    }
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