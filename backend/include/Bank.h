#pragma once

#include "QueueManager.h"
#include <cstddef>
#include <deque>
#include <string>
#include <vector>
#include <cstdint>

class Bank {
public:
    Customer addCustomer(const std::string& name, CustomerType type, int priority);
    bool serveNext(Customer& served, int deskId = 0);
    bool callNext(Customer& called, int deskId = 0);
    bool startService(Customer& customer, int deskId);
    bool completeService(Customer& customer, int deskId);
    bool skipCustomer(Customer& customer, int deskId);
    bool returnSkipped(Customer& customer, int deskId);
    void refreshServiceStates();
    bool peekNext(Customer& next, int deskId = 0) const { return queues_.peekNext(next, deskId); }
    void reset();

    const QueueManager& queues() const { return queues_; }
    QueueManager& queues() { return queues_; }
    const std::deque<Customer>& history() const { return history_; }
    std::size_t servedCount() const { return servedCount_; }

private:
    QueueManager queues_;
    std::deque<Customer> history_;
    std::size_t servedCount_{0};
};
