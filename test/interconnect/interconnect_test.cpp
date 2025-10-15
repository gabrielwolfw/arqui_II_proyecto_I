#include "../../src/interconnect/interconnect.hpp"
#include <cassert>
#include <iostream>
#include <memory>

void test_round_robin() {
    std::cout << "Testing round-robin arbitration..." << std::endl;
    
    auto ram = std::make_shared<RAM>(true);  // Using default 4KB RAM
    Interconnect interconnect(ram, true);
    
    // Create test transactions from different PEs
    // Using valid addresses within 0x0000 to 0x01FF range
    BusTransaction t1{BusTransactionType::BusRd, 0x0100, 0, 0};
    BusTransaction t2{BusTransactionType::BusWB, 0x0150, 1, 0xDEADBEEF};
    BusTransaction t3{BusTransactionType::BusRdX, 0x01A0, 2, 0};
    BusTransaction t4{BusTransactionType::BusUpgr, 0x01F0, 3, 0};
    
    // Add transactions in different order
    interconnect.addRequest(t2);  // PE1
    interconnect.addRequest(t1);  // PE0
    interconnect.addRequest(t4);  // PE3
    interconnect.addRequest(t3);  // PE2
    
    // Process all transactions
    while (interconnect.hasPendingTransactions()) {
        interconnect.processNextTransaction();
    }
    
    // Verify processed transactions
    const auto& processed = interconnect.getProcessedTransactions();
    assert(processed.size() == 4);
    
    // Verify round-robin order (should start from PE0 regardless of add order)
    assert(processed[0].pe_id == 0);
    assert(processed[1].pe_id == 1);
    assert(processed[2].pe_id == 2);
    assert(processed[3].pe_id == 3);
    
    std::cout << "Round-robin arbitration test passed!" << std::endl;
}

void test_memory_operations() {
    std::cout << "Testing memory operations..." << std::endl;
    
    auto ram = std::make_shared<RAM>(true);
    Interconnect interconnect(ram, true);
    
    // Test with valid address within 0x0000 to 0x01FF range
    BusTransaction write{BusTransactionType::BusWB, 0x0100, 0, 0xCAFEBABE};
    interconnect.addRequest(write);
    interconnect.processNextTransaction();
    
    // Read back from the same address
    BusTransaction read{BusTransactionType::BusRd, 0x0100, 1, 0};
    interconnect.addRequest(read);
    interconnect.processNextTransaction();
    
    const auto& processed = interconnect.getProcessedTransactions();
    assert(processed.size() == 2);
    assert(processed[1].data == 0xCAFEBABE);
    
    std::cout << "Memory operations test passed!" << std::endl;
}