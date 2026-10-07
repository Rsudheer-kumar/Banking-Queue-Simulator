#include "Desk.h"
#include <stdexcept>

Desk::Desk(int id, const std::string& name)
    : id_(id), name_(name) {}

void Desk::enqueueNormal(const Customer& customer) {
    normalQueue_.push(customer);
    ++version_;
}

void Desk::enqueueVip(const Customer& customer) {
    vipQueue_.push(customer);
    ++version_;
}

bool Desk::hasWaitingCustomers() const {
    return !vipQueue_.empty() || !normalQueue_.empty();
}

bool Desk::hasVipCustomers() const {
    return !vipQueue_.empty();
}

bool Desk::hasNormalCustomers() const {
    return !normalQueue_.empty();
}

bool Desk::peekNext(Customer& customer) const {
    if (!vipQueue_.empty()) {
        customer = vipQueue_.top();
        return true;
    }
    if (!normalQueue_.empty()) {
        customer = normalQueue_.front();
        return true;
    }
    return false;
}

Customer Desk::serveNext() {
    if (!vipQueue_.empty()) {
        Customer served = vipQueue_.top();
        vipQueue_.pop();
        ++servedCount_;
        ++version_;
        return served;
    }
    if (!normalQueue_.empty()) {
        Customer served = normalQueue_.front();
        normalQueue_.pop();
        ++servedCount_;
        ++version_;
        return served;
    }
    throw std::runtime_error("No customers are waiting at " + name_ + ".");
}

std::vector<Customer> Desk::normalCustomers() const {
    std::vector<Customer> result;
    auto copy = normalQueue_;
    while (!copy.empty()) {
        result.push_back(copy.front());
        copy.pop();
    }
    return result;
}

std::vector<Customer> Desk::vipCustomers() const {
    std::vector<Customer> result;
    auto copy = vipQueue_;
    while (!copy.empty()) {
        result.push_back(copy.top());
        copy.pop();
    }
    return result;
}

void Desk::clear() {
    while (!normalQueue_.empty()) normalQueue_.pop();
    while (!vipQueue_.empty()) vipQueue_.pop();
    servedCount_ = 0;
    ++version_;
}
