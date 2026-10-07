#pragma once

#include "Customer.h"
#include "Desk.h"
#include <cstddef>
#include <queue>
#include <string>
#include <vector>

struct DeskEntry {
    std::size_t load{0};
    int deskId{1};
    unsigned long long version{1};
};

struct DeskCompare {
    bool operator()(const DeskEntry& left, const DeskEntry& right) const {
        if (left.load != right.load) {
            return left.load > right.load; // Min-Heap: smaller workload has higher priority at top
        }
        return left.deskId > right.deskId; // Deterministic tie-breaker: smaller deskId has higher priority
    }
};

class QueueManager {
public:
    QueueManager();

    Customer addCustomer(const std::string& name, CustomerType type, int priority);
    Customer serveNext(int deskId = 0);
    bool peekNext(Customer& next, int deskId = 0) const;

    bool hasWaitingCustomers() const;
    bool hasWaitingCustomers(int deskId) const;

    void clear();

    const std::vector<Desk>& desks() const { return desks_; }
    const Desk& desk(int deskId) const;

    std::size_t normalSize() const;
    std::size_t vipSize() const;
    std::size_t totalWaiting() const;

    int getLeastLoadedDeskId();
    std::vector<DeskEntry> currentHeapState();

    // Compatibility methods for single-desk inspections
    std::vector<Customer> normalCustomers() const;
    std::vector<Customer> vipCustomers() const;
    bool hasVipCustomers() const;
    bool hasNormalCustomers() const;

private:
    void initDesks();
    void syncHeapTop();

    std::vector<Desk> desks_;
    std::priority_queue<DeskEntry, std::vector<DeskEntry>, DeskCompare> deskHeap_;

    unsigned int nextNormalToken_{1};
    unsigned int nextVipToken_{1};
    unsigned long long nextArrivalOrder_{1};
};
