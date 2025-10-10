#include <gtest/gtest.h>
#include "../interconnect/interconnect.hpp"

class InterconnectTest : public ::testing::Test {
protected:
    Interconnect* interconnect;
    
    void SetUp() override {
        // Crear un interconnect con 4 PEs y modo verbose activado
        interconnect = new Interconnect(4, true);
    }
    
    void TearDown() override {
        delete interconnect;
    }
};

TEST_F(InterconnectTest, TestRoundRobinArbitration) {
    // Crear varias transacciones de diferentes PEs
    BusTransaction trans1(0x1000, 0, BusOpType::BusRd);
    BusTransaction trans2(0x2000, 1, BusOpType::BusRdX);
    BusTransaction trans3(0x3000, 2, BusOpType::BusUpgr);
    BusTransaction trans4(0x4000, 3, BusOpType::BusWB);
    
    // Añadir las transacciones
    interconnect->addRequest(trans1);
    interconnect->addRequest(trans2);
    interconnect->addRequest(trans3);
    interconnect->addRequest(trans4);
    
    // Ejecutar 4 ciclos de arbitración
    for (int i = 0; i < 4; i++) {
        interconnect->arbitrate();
    }
    
    // Verificar que todas las transacciones fueron procesadas en orden round-robin
    const auto& processed = interconnect->getProcessedRequests().getTransactions();
    ASSERT_EQ(processed.size(), 4);
    
    // Verificar el orden de procesamiento
    EXPECT_EQ(processed[0].pe_id, 0);
    EXPECT_EQ(processed[1].pe_id, 1);
    EXPECT_EQ(processed[2].pe_id, 2);
    EXPECT_EQ(processed[3].pe_id, 3);
}

TEST_F(InterconnectTest, TestBusTransactionTypes) {
    // Probar todos los tipos de transacciones
    BusTransaction read(0x1000, 0, BusOpType::BusRd);
    BusTransaction readX(0x2000, 1, BusOpType::BusRdX);
    BusTransaction upgrade(0x3000, 2, BusOpType::BusUpgr);
    BusTransaction writeBack(0x4000, 3, BusOpType::BusWB);
    
    interconnect->addRequest(read);
    interconnect->addRequest(readX);
    interconnect->addRequest(upgrade);
    interconnect->addRequest(writeBack);
    
    // Verificar que las transacciones están pendientes
    const auto& pending = interconnect->getPendingRequests().getTransactions();
    ASSERT_EQ(pending.size(), 4);
    
    // Verificar los tipos de operaciones
    EXPECT_EQ(pending[0].op_type, BusOpType::BusRd);
    EXPECT_EQ(pending[1].op_type, BusOpType::BusRdX);
    EXPECT_EQ(pending[2].op_type, BusOpType::BusUpgr);
    EXPECT_EQ(pending[3].op_type, BusOpType::BusWB);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}