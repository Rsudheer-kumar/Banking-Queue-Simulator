#include "Bank.h"

Customer Bank::addCustomer(const std::string& name, CustomerType type, int priority) {
    return queues_.addCustomer(name, type, priority);
}

bool Bank::serveNext(Customer& served, int deskId) {
    if (deskId > 0) {
        if (!queues_.hasWaitingCustomers(deskId)) return false;
    } else {
        if (!queues_.hasWaitingCustomers()) return false;
    }

    served = queues_.serveNext(deskId);
    served.servedTime = currentFormattedTime();
    ++servedCount_;
    history_.push_front(served);
    if (history_.size() > 10) history_.pop_back();
    return true;
}

void Bank::refreshServiceStates() { queues_.refreshServiceStates(); }
bool Bank::callNext(Customer& called, int deskId) { return queues_.callNext(called, deskId); }
bool Bank::startService(Customer& customer, int deskId) { return queues_.startService(customer, deskId); }
bool Bank::completeService(Customer& customer, int deskId) {
    if (!queues_.completeService(customer, deskId)) return false;
    ++servedCount_;
    history_.push_front(customer);
    if (history_.size() > 10) history_.pop_back();
    return true;
}
bool Bank::skipCustomer(Customer& customer, int deskId) { return queues_.skipCustomer(customer, deskId); }
bool Bank::returnSkipped(Customer& customer, int deskId) { return queues_.returnSkipped(customer, deskId); }

void Bank::reset() {
    queues_.clear();
    history_.clear();
    servedCount_ = 0;
}
