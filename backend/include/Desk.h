#pragma once

#include "Customer.h"
#include <cstddef>
#include <queue>
#include <string>
#include <vector>

class Desk {
public:
    Desk(int id, const std::string& name);

    int id() const { return id_; }
    const std::string& name() const { return name_; }

    void enqueueNormal(const Customer& customer);
    void enqueueVip(const Customer& customer);

    bool hasWaitingCustomers() const;
    bool hasVipCustomers() const;
    bool hasNormalCustomers() const;
    bool peekNext(Customer& customer) const;
    Customer serveNext();

    std::size_t normalWaiting() const { return normalQueue_.size(); }
    std::size_t vipWaiting() const { return vipQueue_.size(); }
    std::size_t totalWaiting() const { return normalQueue_.size() + vipQueue_.size(); }
    std::size_t servedCount() const { return servedCount_; }
    unsigned long long version() const { return version_; }

    std::vector<Customer> normalCustomers() const;
    std::vector<Customer> vipCustomers() const;

    void clear();

private:
    struct VipCompare {
        bool operator()(const Customer& left, const Customer& right) const {
            if (left.priority != right.priority) {
                return left.priority > right.priority; // Lower number means higher priority (1=High)
            }
            return left.arrivalOrder > right.arrivalOrder; // FIFO for equal priorities
        }
    };

    int id_;
    std::string name_;
    std::queue<Customer> normalQueue_;
    std::priority_queue<Customer, std::vector<Customer>, VipCompare> vipQueue_;
    std::size_t servedCount_{0};
    unsigned long long version_{1};
};
