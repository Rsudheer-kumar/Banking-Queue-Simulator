#include "Bank.h"
#include <cassert>
#include <iostream>

int main() {
    Bank bank;
    bank.addCustomer("Grace", CustomerType::Normal, 0);

    Customer customer;
    assert(bank.callNext(customer, 1));
    assert(customer.serviceState == ServiceState::Called);
    assert(bank.queues().desk(1).hasActiveCustomer());
    assert(!bank.callNext(customer, 1));

    assert(bank.startService(customer, 1));
    assert(customer.serviceState == ServiceState::InService);
    assert(bank.completeService(customer, 1));
    assert(customer.serviceState == ServiceState::Completed);
    assert(bank.servedCount() == 1);
    assert(bank.history().front().token == "N-001");

    const Customer noShow = bank.addCustomer("No Show", CustomerType::Normal, 0);
    assert(bank.callNext(customer, noShow.assignedDeskId));
    assert(bank.skipCustomer(customer, noShow.assignedDeskId));
    assert(customer.serviceState == ServiceState::Skipped);
    assert(bank.returnSkipped(customer, noShow.assignedDeskId));
    assert(!bank.queues().desk(noShow.assignedDeskId).hasActiveCustomer());
    assert(bank.queues().desk(noShow.assignedDeskId).normalWaiting() == 1);

    bank.reset();
    assert(!bank.queues().desk(1).hasActiveCustomer());
    assert(!bank.queues().desk(2).hasActiveCustomer());
    assert(bank.history().empty());
    assert(bank.servedCount() == 0);

    std::cout << "[PASS] Customer call, service, completion, skip, return, and reset\n";
    return 0;
}
