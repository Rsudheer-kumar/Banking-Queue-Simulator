#include "Bank.h"
#include <cassert>
#include <iostream>
#include <vector>

void testQueueBasicOperations() {
    QueueManager qm;
    assert(!qm.hasWaitingCustomers());
    assert(qm.normalSize() == 0);
    assert(qm.vipSize() == 0);

    // Test Normal Queue insertion (push/enqueue)
    auto c1 = qm.addCustomer("Amit", CustomerType::Normal, 0);
    auto c2 = qm.addCustomer("Ravi", CustomerType::Normal, 0);
    auto c3 = qm.addCustomer("Kiran", CustomerType::Normal, 0);

    assert(c1.token == "N-001");
    assert(c2.token == "N-002");
    assert(c3.token == "N-003");
    assert(qm.normalSize() == 3);
    assert(qm.hasNormalCustomers());
    assert(!qm.hasVipCustomers());

    // Test front inspection via normalCustomers()
    auto normalList = qm.normalCustomers();
    assert(normalList.size() == 3);
    assert(normalList[0].name == "Amit");
    assert(normalList[1].name == "Ravi");
    assert(normalList[2].name == "Kiran");

    // Test peekNext
    Customer peeked;
    assert(qm.peekNext(peeked));
    assert(peeked.token == "N-001" && peeked.name == "Amit");

    // Test removal (pop/dequeue) - FIFO
    auto s1 = qm.serveNext();
    assert(s1.token == "N-001" && s1.name == "Amit");
    assert(qm.normalSize() == 2);

    auto s2 = qm.serveNext();
    assert(s2.token == "N-002" && s2.name == "Ravi");

    auto s3 = qm.serveNext();
    assert(s3.token == "N-003" && s3.name == "Kiran");
    assert(qm.normalSize() == 0);
    assert(!qm.hasWaitingCustomers());

    std::cout << "[PASS] Normal Queue operations (push, front, pop, FIFO)\n";
}

void testPriorityQueueOperations() {
    QueueManager qm;

    // Test VIP insertion with different priorities
    // Priority: 1 = High, 2 = Medium, 3 = Low
    auto vLow = qm.addCustomer("Arjun", CustomerType::Vip, 3);
    auto vHigh = qm.addCustomer("Rahul", CustomerType::Vip, 1);
    auto vMed = qm.addCustomer("Priya", CustomerType::Vip, 2);

    assert(vLow.token == "V-001");
    assert(vHigh.token == "V-002");
    assert(vMed.token == "V-003");
    assert(qm.vipSize() == 3);

    // Test peekNext shows highest priority (top)
    Customer peeked;
    assert(qm.peekNext(peeked));
    assert(peeked.token == "V-002" && peeked.name == "Rahul" && peeked.priority == 1);

    // Test vipCustomers() ordering: High (1) -> Medium (2) -> Low (3)
    auto vipList = qm.vipCustomers();
    assert(vipList.size() == 3);
    assert(vipList[0].name == "Rahul" && vipList[0].priority == 1);
    assert(vipList[1].name == "Priya" && vipList[1].priority == 2);
    assert(vipList[2].name == "Arjun" && vipList[2].priority == 3);

    // Test serveNext pops in priority order
    auto s1 = qm.serveNext();
    assert(s1.name == "Rahul" && s1.priority == 1);

    auto s2 = qm.serveNext();
    assert(s2.name == "Priya" && s2.priority == 2);

    auto s3 = qm.serveNext();
    assert(s3.name == "Arjun" && s3.priority == 3);
    assert(qm.vipSize() == 0);

    // Test tie-breaking among equal priorities (FIFO by arrival order)
    qm.clear();
    auto vMed1 = qm.addCustomer("VIP First", CustomerType::Vip, 2);
    auto vMed2 = qm.addCustomer("VIP Second", CustomerType::Vip, 2);
    assert(qm.serveNext().name == "VIP First");
    assert(qm.serveNext().name == "VIP Second");

    std::cout << "[PASS] VIP Priority Queue operations (push, top, pop, priorities, tie-breaking)\n";
}

void testServeNextAlgorithm() {
    QueueManager qm;

    // Rule: VIP first, then normal, else none
    qm.addCustomer("Normal 1", CustomerType::Normal, 0);
    qm.addCustomer("VIP 1", CustomerType::Vip, 2);
    qm.addCustomer("Normal 2", CustomerType::Normal, 0);

    // Should serve VIP first
    assert(qm.serveNext().name == "VIP 1");
    // Then Normal 1
    assert(qm.serveNext().name == "Normal 1");
    // Then Normal 2
    assert(qm.serveNext().name == "Normal 2");

    // Empty throws exception
    bool threw = false;
    try {
        qm.serveNext();
    } catch (const std::exception&) {
        threw = true;
    }
    assert(threw);

    std::cout << "[PASS] Service algorithm (VIP prioritized over Normal, empty check)\n";
}

void testBankHistoryAndReset() {
    Bank bank;
    bank.addCustomer("Cust A", CustomerType::Normal, 0);
    bank.addCustomer("Cust B", CustomerType::Vip, 1);

    Customer served;
    assert(bank.serveNext(served));
    assert(served.name == "Cust B");
    assert(!served.servedTime.empty());
    assert(bank.servedCount() == 1);
    assert(bank.history().size() == 1);
    assert(bank.history().front().name == "Cust B");

    assert(bank.serveNext(served));
    assert(served.name == "Cust A");
    assert(bank.servedCount() == 2);
    assert(bank.history().size() == 2);
    // Newest served is first
    assert(bank.history().front().name == "Cust A");

    // Test reset
    bank.reset();
    assert(bank.queues().normalSize() == 0);
    assert(bank.queues().vipSize() == 0);
    assert(bank.history().empty());
    assert(bank.servedCount() == 0);

    // After reset, token numbers start back at 1
    auto newN = bank.addCustomer("Fresh Normal", CustomerType::Normal, 0);
    auto newV = bank.addCustomer("Fresh VIP", CustomerType::Vip, 1);
    assert(newN.token == "N-001");
    assert(newV.token == "V-001");

    std::cout << "[PASS] Bank history tracking, timestamps, and reset\n";
}

void testPromptRealScenario() {
    // Exact scenario from prompt requirement 22:
    // Step 1: Reset simulator
    Bank bank;
    bank.reset();

    // Step 2: Add N-001 Amit, N-002 Ravi, N-003 Kiran
    auto n1 = bank.addCustomer("Amit", CustomerType::Normal, 0);
    auto n2 = bank.addCustomer("Ravi", CustomerType::Normal, 0);
    auto n3 = bank.addCustomer("Kiran", CustomerType::Normal, 0);
    assert(n1.token == "N-001");
    assert(n2.token == "N-002");
    assert(n3.token == "N-003");

    // Step 3: Add V-001 Rahul (High), V-002 Priya (Med), V-003 Arjun (Low)
    auto v1 = bank.addCustomer("Rahul", CustomerType::Vip, 1);
    auto v2 = bank.addCustomer("Priya", CustomerType::Vip, 2);
    auto v3 = bank.addCustomer("Arjun", CustomerType::Vip, 3);
    assert(v1.token == "V-001");
    assert(v2.token == "V-002");
    assert(v3.token == "V-003");

    // Step 4: Verify displayed queues
    assert(bank.queues().normalSize() == 3);
    assert(bank.queues().vipSize() == 3);
    auto normalQ = bank.queues().normalCustomers();
    assert(normalQ[0].token == "N-001" && normalQ[1].token == "N-002" && normalQ[2].token == "N-003");

    auto vipQ = bank.queues().vipCustomers();
    assert(vipQ[0].token == "V-001" && vipQ[1].token == "V-002" && vipQ[2].token == "V-003");

    // Step 5: Verify next customer. Expected: V-001 Rahul
    Customer nextCust;
    assert(bank.peekNext(nextCust));
    assert(nextCust.token == "V-001" && nextCust.name == "Rahul");

    // Step 6: Serve repeatedly. Expected: V-001, V-002, V-003, N-001, N-002, N-003
    Customer s;
    std::vector<std::string> servedTokens;
    while (bank.serveNext(s)) {
        servedTokens.push_back(s.token);
    }
    assert(servedTokens.size() == 6);
    assert(servedTokens[0] == "V-001");
    assert(servedTokens[1] == "V-002");
    assert(servedTokens[2] == "V-003");
    assert(servedTokens[3] == "N-001");
    assert(servedTokens[4] == "N-002");
    assert(servedTokens[5] == "N-003");

    // Step 7: Verify final state: Normal = 0, VIP = 0, Total = 0
    assert(bank.queues().normalSize() == 0);
    assert(bank.queues().vipSize() == 0);
    assert(bank.queues().normalSize() + bank.queues().vipSize() == 0);

    // Step 8: Verify history contains all six customers, newest first
    assert(bank.history().size() == 6);
    assert(bank.history()[0].token == "N-003");
    assert(bank.history()[1].token == "N-002");
    assert(bank.history()[2].token == "N-001");
    assert(bank.history()[3].token == "V-003");
    assert(bank.history()[4].token == "V-002");
    assert(bank.history()[5].token == "V-001");

    std::cout << "[PASS] Prompt Requirement 22 Real Scenario (Amit, Ravi, Kiran, Rahul, Priya, Arjun)\n";
}

void testSmartCounterAllocation() {
    QueueManager qm;
    for (int i = 0; i < 20; ++i) {
        qm.addCustomer("Seed " + std::to_string(i), CustomerType::Normal, 0);
    }

    // Shape the four desks to 5, 2, 4, 1 through real desk service.
    qm.serveNext(2);
    qm.serveNext(2);
    qm.serveNext(2);
    qm.serveNext(3);
    qm.serveNext(4);
    qm.serveNext(4);
    qm.serveNext(4);
    qm.serveNext(4);

    assert(qm.desk(1).totalWaiting() == 5);
    assert(qm.desk(2).totalWaiting() == 2);
    assert(qm.desk(3).totalWaiting() == 4);
    assert(qm.desk(4).totalWaiting() == 1);
    auto selected = qm.addCustomer("Least Loaded", CustomerType::Normal, 0);
    assert(selected.assignedDeskId == 4);

    // Desk 02 and Desk 04 are now tied at two; lower ID wins.
    auto tieWinner = qm.addCustomer("Tie Winner", CustomerType::Normal, 0);
    assert(tieWinner.assignedDeskId == 2);

    // Repeated updates must discard stale heap entries.
    qm.clear();
    for (int i = 0; i < 4; ++i) {
        auto customer = qm.addCustomer("Critical " + std::to_string(i), CustomerType::Normal, 0);
        assert(customer.assignedDeskId == i + 1);
    }
    auto next = qm.addCustomer("Critical next", CustomerType::Normal, 0);
    assert(next.assignedDeskId == 1);

    std::cout << "[PASS] Smart counter allocation, tie-breaking, and lazy heap updates\n";
}

void testPerDeskVipQueues() {
    QueueManager qm;
    auto first = qm.addCustomer("Desk One VIP", CustomerType::Vip, 3);
    auto second = qm.addCustomer("Desk Two VIP", CustomerType::Vip, 1);
    assert(first.assignedDeskId == 1);
    assert(second.assignedDeskId == 2);
    qm.addCustomer("Desk Three Normal", CustomerType::Normal, 0);
    qm.addCustomer("Desk Four Normal", CustomerType::Normal, 0);
    qm.addCustomer("Desk One High", CustomerType::Vip, 1);
    assert(qm.desk(1).vipWaiting() == 2);
    assert(qm.desk(2).vipWaiting() == 1);
    assert(qm.serveNext(1).name == "Desk One High");
    assert(qm.serveNext(2).name == "Desk Two VIP");

    std::cout << "[PASS] Independent per-desk VIP priority queues\n";
}

int main() {
    std::cout << "Running Bank Queue Simulator Unit Tests...\n";
    testQueueBasicOperations();
    testPriorityQueueOperations();
    testServeNextAlgorithm();
    testBankHistoryAndReset();
    testPromptRealScenario();
    testSmartCounterAllocation();
    testPerDeskVipQueues();
    std::cout << "\n>>> ALL C++ DSA UNIT TESTS PASSED SUCCESSFULLY! <<<\n";
    return 0;
}
