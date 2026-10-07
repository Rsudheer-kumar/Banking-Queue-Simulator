#include "QueueManager.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <chrono>

namespace {
std::int64_t nowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
}

QueueManager::QueueManager() {
    initDesks();
}

void QueueManager::initDesks() {
    desks_.clear();
    while (!deskHeap_.empty()) deskHeap_.pop();

    for (int i = 1; i <= 4; ++i) {
        std::ostringstream name;
        name << "Desk " << std::setfill('0') << std::setw(2) << i;
        desks_.emplace_back(i, name.str());
        deskHeap_.push({0, i, desks_.back().version()});
    }
}

void QueueManager::syncHeapTop() {
    while (!deskHeap_.empty()) {
        const DeskEntry top = deskHeap_.top();
        const int idx = top.deskId - 1;
        if (idx >= 0 && idx < static_cast<int>(desks_.size())) {
            const Desk& d = desks_[idx];
            if (top.version == d.version() && top.load == d.totalWaiting()) {
                return; // Valid current entry at top
            }
        }
        deskHeap_.pop(); // Discard stale entry
    }
}

int QueueManager::getLeastLoadedDeskId() {
    syncHeapTop();
    if (!deskHeap_.empty()) {
        return deskHeap_.top().deskId;
    }
    // Fallback if heap somehow emptied
    return 1;
}

std::vector<DeskEntry> QueueManager::currentHeapState() {
    std::vector<DeskEntry> result;
    auto copy = deskHeap_;
    std::vector<bool> seen(desks_.size() + 1, false);

    while (!copy.empty() && result.size() < desks_.size()) {
        DeskEntry top = copy.top();
        copy.pop();
        int idx = top.deskId - 1;
        if (idx >= 0 && idx < static_cast<int>(desks_.size())) {
            const Desk& d = desks_[idx];
            if (top.version == d.version() && top.load == d.totalWaiting() && !seen[top.deskId]) {
                result.push_back(top);
                seen[top.deskId] = true;
            }
        }
    }
    // In case any desk was missing from copy
    for (const auto& d : desks_) {
        if (!seen[d.id()]) {
            result.push_back({d.totalWaiting(), d.id(), d.version()});
            seen[d.id()] = true;
        }
    }
    return result;
}

Customer QueueManager::addCustomer(const std::string& name, CustomerType type, int priority) {
    auto start = name.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) throw std::invalid_argument("Customer name cannot be empty.");
    auto end = name.find_last_not_of(" \t\r\n");
    std::string trimmedName = name.substr(start, end - start + 1);

    if (type == CustomerType::Vip && (priority < 1 || priority > 3)) {
        throw std::invalid_argument("VIP priority must be between 1 and 3.");
    }
    if (type == CustomerType::Normal) priority = 0;

    // Step 2 & 3: Find least-loaded desk via C++ Min-Heap
    int targetDeskId = getLeastLoadedDeskId();
    Desk& targetDesk = desks_[targetDeskId - 1];

    Customer customer;
    customer.name = trimmedName;
    customer.type = type;
    customer.priority = priority;
    customer.arrivalOrder = nextArrivalOrder_++;
    customer.assignedDeskId = targetDeskId;
    customer.assignedDeskName = targetDesk.name();

    std::ostringstream token;
    token << (type == CustomerType::Vip ? "V-" : "N-")
          << std::setfill('0') << std::setw(3)
          << (type == CustomerType::Vip ? nextVipToken_++ : nextNormalToken_++);
    customer.token = token.str();

    // Step 5: Insert customer into that desk's actual DSA
    if (type == CustomerType::Vip) {
        targetDesk.enqueueVip(customer);
    } else {
        targetDesk.enqueueNormal(customer);
    }

    // Step 7: Push new DeskEntry into the desk Min-Heap
    deskHeap_.push({targetDesk.totalWaiting(), targetDesk.id(), targetDesk.version()});

    return customer;
}

Customer QueueManager::serveNext(int deskId) {
    if (deskId >= 1 && deskId <= static_cast<int>(desks_.size())) {
        Desk& targetDesk = desks_[deskId - 1];
        if (!targetDesk.hasWaitingCustomers()) {
            throw std::runtime_error("No customers are currently waiting at " + targetDesk.name() + ".");
        }

        Customer served = targetDesk.serveNext();
        deskHeap_.push({targetDesk.totalWaiting(), targetDesk.id(), targetDesk.version()});
        return served;
    }

    // If deskId == 0, find the desk with the most urgent customer across all desks
    Customer bestCandidate;
    int bestDeskId = 0;
    bool foundVip = false;

    for (auto& d : desks_) {
        Customer candidate;
        if (d.peekNext(candidate)) {
            if (candidate.type == CustomerType::Vip) {
                if (!foundVip) {
                    bestCandidate = candidate;
                    bestDeskId = d.id();
                    foundVip = true;
                } else if (candidate.priority < bestCandidate.priority ||
                          (candidate.priority == bestCandidate.priority && d.id() < bestDeskId)) {
                    bestCandidate = candidate;
                    bestDeskId = d.id();
                }
            } else if (!foundVip) {
                if (bestDeskId == 0) {
                    bestCandidate = candidate;
                    bestDeskId = d.id();
                }
            }
        }
    }

    if (bestDeskId == 0) {
        throw std::runtime_error("No customers are currently waiting.");
    }

    Desk& targetDesk = desks_[bestDeskId - 1];
    Customer served = targetDesk.serveNext();
    deskHeap_.push({targetDesk.totalWaiting(), targetDesk.id(), targetDesk.version()});
    return served;
}

void QueueManager::refreshServiceStates() {
    const auto now = nowSeconds();
    for (auto& desk : desks_) desk.expireActive(now);
}

bool QueueManager::callNext(Customer& called, int deskId) {
    refreshServiceStates();
    int target = deskId;
    if (target < 1) {
        Customer candidate;
        int bestDesk = 0;
        bool foundVip = false;
        for (const auto& desk : desks_) {
            if (desk.hasActiveCustomer() || !desk.peekNext(candidate)) continue;
            if (!foundVip && (bestDesk == 0 || candidate.type == CustomerType::Vip)) {
                bestDesk = desk.id();
                foundVip = candidate.type == CustomerType::Vip;
            } else if (candidate.type == CustomerType::Vip && foundVip) {
                Customer best;
                desks_[bestDesk - 1].peekNext(best);
                if (candidate.priority < best.priority ||
                    (candidate.priority == best.priority && desk.id() < bestDesk)) {
                    bestDesk = desk.id();
                }
            }
        }
        target = bestDesk;
    }
    if (target < 1 || target > static_cast<int>(desks_.size())) return false;
    const auto now = nowSeconds();
    const bool result = desks_[target - 1].callNext(called, now, now + 60);
    if (result) deskHeap_.push({desks_[target - 1].totalWaiting(), target, desks_[target - 1].version()});
    return result;
}

bool QueueManager::startService(Customer& customer, int deskId) {
    refreshServiceStates();
    if (deskId < 1 || deskId > static_cast<int>(desks_.size())) return false;
    return desks_[deskId - 1].startService(customer, nowSeconds());
}

bool QueueManager::completeService(Customer& customer, int deskId) {
    if (deskId < 1 || deskId > static_cast<int>(desks_.size())) return false;
    return desks_[deskId - 1].completeService(customer);
}

bool QueueManager::skipCustomer(Customer& customer, int deskId) {
    refreshServiceStates();
    if (deskId < 1 || deskId > static_cast<int>(desks_.size())) return false;
    return desks_[deskId - 1].skipActive(customer);
}

bool QueueManager::returnSkipped(Customer& customer, int deskId) {
    if (deskId < 1 || deskId > static_cast<int>(desks_.size())) return false;
    const bool result = desks_[deskId - 1].returnSkipped(customer);
    if (result) deskHeap_.push({desks_[deskId - 1].totalWaiting(), deskId, desks_[deskId - 1].version()});
    return result;
}

bool QueueManager::peekNext(Customer& next, int deskId) const {
    if (deskId >= 1 && deskId <= static_cast<int>(desks_.size())) {
        return desks_[deskId - 1].peekNext(next);
    }

    Customer bestCandidate;
    int bestDeskId = 0;
    bool foundVip = false;

    for (const auto& d : desks_) {
        Customer candidate;
        if (d.peekNext(candidate)) {
            if (candidate.type == CustomerType::Vip) {
                if (!foundVip) {
                    bestCandidate = candidate;
                    bestDeskId = d.id();
                    foundVip = true;
                } else if (candidate.priority < bestCandidate.priority ||
                          (candidate.priority == bestCandidate.priority && d.id() < bestDeskId)) {
                    bestCandidate = candidate;
                    bestDeskId = d.id();
                }
            } else if (!foundVip) {
                if (bestDeskId == 0) {
                    bestCandidate = candidate;
                    bestDeskId = d.id();
                }
            }
        }
    }

    if (bestDeskId != 0) {
        next = bestCandidate;
        return true;
    }
    return false;
}

bool QueueManager::hasWaitingCustomers() const {
    for (const auto& d : desks_) {
        if (d.hasWaitingCustomers()) return true;
    }
    return false;
}

bool QueueManager::hasWaitingCustomers(int deskId) const {
    if (deskId >= 1 && deskId <= static_cast<int>(desks_.size())) {
        return desks_[deskId - 1].hasWaitingCustomers();
    }
    return false;
}

bool QueueManager::hasVipCustomers() const {
    for (const auto& d : desks_) {
        if (d.hasVipCustomers()) return true;
    }
    return false;
}

bool QueueManager::hasNormalCustomers() const {
    for (const auto& d : desks_) {
        if (d.hasNormalCustomers()) return true;
    }
    return false;
}

const Desk& QueueManager::desk(int deskId) const {
    if (deskId >= 1 && deskId <= static_cast<int>(desks_.size())) {
        return desks_[deskId - 1];
    }
    throw std::out_of_range("Invalid desk ID: " + std::to_string(deskId));
}

std::size_t QueueManager::normalSize() const {
    std::size_t sum = 0;
    for (const auto& d : desks_) sum += d.normalWaiting();
    return sum;
}

std::size_t QueueManager::vipSize() const {
    std::size_t sum = 0;
    for (const auto& d : desks_) sum += d.vipWaiting();
    return sum;
}

std::size_t QueueManager::totalWaiting() const {
    std::size_t sum = 0;
    for (const auto& d : desks_) sum += d.totalWaiting();
    return sum;
}

std::vector<Customer> QueueManager::normalCustomers() const {
    std::vector<Customer> allNormal;
    for (const auto& d : desks_) {
        auto list = d.normalCustomers();
        allNormal.insert(allNormal.end(), list.begin(), list.end());
    }
    std::sort(allNormal.begin(), allNormal.end(), [](const Customer& left, const Customer& right) {
        return left.arrivalOrder < right.arrivalOrder;
    });
    return allNormal;
}

std::vector<Customer> QueueManager::vipCustomers() const {
    std::vector<Customer> allVip;
    for (const auto& d : desks_) {
        auto list = d.vipCustomers();
        allVip.insert(allVip.end(), list.begin(), list.end());
    }
    std::sort(allVip.begin(), allVip.end(), [](const Customer& left, const Customer& right) {
        if (left.priority != right.priority) return left.priority < right.priority;
        return left.arrivalOrder < right.arrivalOrder;
    });
    return allVip;
}

void QueueManager::clear() {
    for (auto& d : desks_) {
        d.clear();
    }
    while (!deskHeap_.empty()) deskHeap_.pop();
    for (const auto& d : desks_) {
        deskHeap_.push({0, d.id(), d.version()});
    }
    nextNormalToken_ = 1;
    nextVipToken_ = 1;
    nextArrivalOrder_ = 1;
}
