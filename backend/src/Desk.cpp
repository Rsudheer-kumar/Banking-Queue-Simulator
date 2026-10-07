#include "Desk.h"
#include <chrono>
#include <stdexcept>

namespace {
std::int64_t nowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
}

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
    hasActiveCustomer_ = false;
    hasSkippedCustomer_ = false;
}

bool Desk::callNext(Customer& customer, std::int64_t now, std::int64_t deadline) {
    if (hasActiveCustomer_ || !peekNext(customer)) return false;
    if (customer.type == CustomerType::Vip) vipQueue_.pop();
    else normalQueue_.pop();
    customer.serviceState = ServiceState::Called;
    customer.callStartedAt = now;
    customer.callDeadline = deadline;
    activeCustomer_ = customer;
    hasActiveCustomer_ = true;
    ++version_;
    return true;
}

void Desk::expireActive(std::int64_t now) {
    if (hasActiveCustomer_ && activeCustomer_.serviceState == ServiceState::Called &&
        activeCustomer_.callDeadline <= now) {
        activeCustomer_.serviceState = ServiceState::Skipped;
        activeCustomer_.callDeadline = 0;
        skippedCustomer_ = activeCustomer_;
        hasSkippedCustomer_ = true;
        hasActiveCustomer_ = false;
    }
}

bool Desk::startService(Customer& customer, std::int64_t now) {
    expireActive(now);
    if (!hasActiveCustomer_ || activeCustomer_.serviceState != ServiceState::Called) return false;
    activeCustomer_.serviceState = ServiceState::InService;
    activeCustomer_.serviceStartedTime = currentFormattedTime();
    activeCustomer_.callDeadline = 0;
    customer = activeCustomer_;
    return true;
}

bool Desk::skipActive(Customer& customer) {
    if (!hasActiveCustomer_ || activeCustomer_.serviceState != ServiceState::Called) return false;
    activeCustomer_.serviceState = ServiceState::Skipped;
    activeCustomer_.callDeadline = 0;
    skippedCustomer_ = activeCustomer_;
    hasSkippedCustomer_ = true;
    customer = activeCustomer_;
    hasActiveCustomer_ = false;
    return true;
}

bool Desk::completeService(Customer& customer) {
    if (!hasActiveCustomer_ || activeCustomer_.serviceState != ServiceState::InService) return false;
    activeCustomer_.serviceState = ServiceState::Completed;
    activeCustomer_.servedTime = currentFormattedTime();
    customer = activeCustomer_;
    hasActiveCustomer_ = false;
    ++servedCount_;
    ++version_;
    return true;
}

bool Desk::returnSkipped(Customer& customer) {
    if (!hasSkippedCustomer_) return false;
    customer = skippedCustomer_;
    customer.serviceState = ServiceState::Waiting;
    customer.callStartedAt = 0;
    customer.callDeadline = 0;
    customer.serviceStartedTime.clear();
    if (customer.type == CustomerType::Vip) vipQueue_.push(customer);
    else normalQueue_.push(customer);
    hasSkippedCustomer_ = false;
    ++version_;
    return true;
}
