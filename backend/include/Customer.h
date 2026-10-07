#pragma once

#include <ctime>
#include <string>

enum class CustomerType { Normal, Vip };

struct Customer {
    std::string token;
    std::string name;
    CustomerType type{CustomerType::Normal};
    int priority{0};
    unsigned long long arrivalOrder{0};
    std::string servedTime; // Populated when served (e.g. "2:31 PM")
    int assignedDeskId{0};          // 1, 2, 3, or 4
    std::string assignedDeskName;   // e.g. "Desk 01"
};

inline std::string customerTypeName(CustomerType type) {
    return type == CustomerType::Vip ? "VIP" : "NORMAL";
}

inline std::string priorityName(int priority) {
    if (priority == 1) return "HIGH";
    if (priority == 2) return "MEDIUM";
    return "LOW";
}

inline std::string currentFormattedTime() {
    std::time_t now = std::time(nullptr);
    std::tm* tmPtr = std::localtime(&now);
    if (!tmPtr) return "";
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%I:%M %p", tmPtr);
    std::string result = buffer;
    if (!result.empty() && result[0] == '0') {
        result.erase(0, 1);
    }
    return result;
}
