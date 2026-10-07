#include "Bank.h"

#include <nlohmann/json.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

namespace {
json customerJson(const Customer& customer) {
    json result{
        {"token", customer.token},
        {"name", customer.name},
        {"type", customerTypeName(customer.type)},
        {"priority", customer.priority},
        {"assignedDesk", customer.assignedDeskName},
        {"assignedDeskId", customer.assignedDeskId}
    };
    if (customer.type == CustomerType::Vip) {
        result["priorityLabel"] = priorityName(customer.priority);
    }
    if (!customer.servedTime.empty()) {
        result["servedTime"] = customer.servedTime;
    }
    return result;
}

json deskJson(const Desk& desk) {
    json result{
        {"id", desk.id()},
        {"name", desk.name()},
        {"normalWaiting", desk.normalWaiting()},
        {"vipWaiting", desk.vipWaiting()},
        {"totalWaiting", desk.totalWaiting()},
        {"servedCount", desk.servedCount()}
    };

    result["normalQueue"] = json::array();
    for (const auto& customer : desk.normalCustomers()) {
        result["normalQueue"].push_back(customerJson(customer));
    }

    result["vipQueue"] = json::array();
    for (const auto& customer : desk.vipCustomers()) {
        result["vipQueue"].push_back(customerJson(customer));
    }

    Customer next;
    if (desk.peekNext(next)) {
        result["nextCustomer"] = customerJson(next);
    } else {
        result["nextCustomer"] = nullptr;
    }
    return result;
}

std::string jsonResponse(const json& body, int status, const std::string& statusText) {
    const std::string payload = (status == 204) ? "" : body.dump();
    std::ostringstream response;
    response << "HTTP/1.1 " << status << " " << statusText << "\r\n"
             << "Content-Type: application/json; charset=utf-8\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
             << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             << "Content-Length: " << payload.size() << "\r\n"
             << "Connection: close\r\n\r\n" << payload;
    return response.str();
}

std::string errorResponse(const std::string& message, int status = 400) {
    std::string statusText = "Bad Request";
    if (status == 404) statusText = "Not Found";
    else if (status == 409) statusText = "Conflict";
    else if (status == 500) statusText = "Internal Server Error";
    return jsonResponse({{"success", false}, {"message", message}}, status, statusText);
}

std::string readRequest(SOCKET client) {
    std::string request;
    char buffer[4096];
    size_t headerEnd = std::string::npos;
    while (headerEnd == std::string::npos && request.size() < 1024 * 1024) {
        const int received = recv(client, buffer, sizeof(buffer), 0);
        if (received <= 0) return {};
        request.append(buffer, received);
        headerEnd = request.find("\r\n\r\n");
    }

    if (headerEnd == std::string::npos) return {};

    // Parse Content-Length case-insensitively
    size_t contentLength = 0;
    std::string lowerHeaders = request.substr(0, headerEnd);
    std::transform(lowerHeaders.begin(), lowerHeaders.end(), lowerHeaders.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const std::string marker = "content-length:";
    const auto markerPosition = lowerHeaders.find(marker);
    if (markerPosition != std::string::npos) {
        const auto lineEnd = lowerHeaders.find("\r\n", markerPosition);
        const auto valStart = markerPosition + marker.size();
        const auto valLen = (lineEnd == std::string::npos ? lowerHeaders.size() : lineEnd) - valStart;
        std::string numStr = lowerHeaders.substr(valStart, valLen);
        const auto firstDigit = numStr.find_first_not_of(" \t");
        if (firstDigit != std::string::npos) {
            try {
                contentLength = static_cast<size_t>(std::stoul(numStr.substr(firstDigit)));
            } catch (...) {
                contentLength = 0;
            }
        }
    }

    const size_t bodyStart = headerEnd + 4;
    while (request.size() - bodyStart < contentLength) {
        const int received = recv(client, buffer, sizeof(buffer), 0);
        if (received <= 0) break;
        request.append(buffer, received);
    }
    return request;
}

std::string handleRequest(Bank& bank, const std::string& request) {
    const auto firstLineEnd = request.find("\r\n");
    if (firstLineEnd == std::string::npos) return errorResponse("Malformed HTTP request.");
    std::istringstream firstLine(request.substr(0, firstLineEnd));
    std::string method;
    std::string path;
    firstLine >> method >> path;

    // Strip query string and trailing slashes for robust matching
    const auto queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        path = path.substr(0, queryPos);
    }
    if (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }

    const auto headerEnd = request.find("\r\n\r\n");
    const std::string body = headerEnd == std::string::npos ? "" : request.substr(headerEnd + 4);

    if (method == "OPTIONS") return jsonResponse({}, 204, "No Content");

    if (method == "GET" && (path == "/" || path == "/health" || path == "/api/health")) {
        return jsonResponse({
            {"success", true},
            {"status", "online"},
            {"service", "Banking Queue Simulator C++ API Server (Smart Counter Allocation)"},
            {"waitingTotal", bank.queues().totalWaiting()},
            {"countersCount", bank.queues().desks().size()}
        }, 200, "OK");
    }

    if (method == "GET" && (path == "/api/queue" || path == "/api/stats" || path == "/api/desks")) {
        json result{
            {"success", true},
            {"normalWaiting", bank.queues().normalSize()},
            {"vipWaiting", bank.queues().vipSize()},
            {"totalWaiting", bank.queues().totalWaiting()},
            {"totalServed", bank.servedCount()}
        };

        // All desks information
        result["desks"] = json::array();
        for (const auto& d : bank.queues().desks()) {
            result["desks"].push_back(deskJson(d));
        }

        // Min-Heap current representation for visualization
        int leastLoadedId = const_cast<Bank&>(bank).queues().getLeastLoadedDeskId();
        result["leastLoadedDeskId"] = leastLoadedId;
        std::ostringstream deskName;
        deskName << "Desk " << (leastLoadedId < 10 ? "0" : "") << leastLoadedId;
        result["leastLoadedDeskName"] = deskName.str();

        auto heapState = const_cast<Bank&>(bank).queues().currentHeapState();
        result["minHeap"] = json::array();
        for (size_t i = 0; i < heapState.size(); ++i) {
            const auto& entry = heapState[i];
            std::ostringstream dName;
            dName << "Desk " << (entry.deskId < 10 ? "0" : "") << entry.deskId;
            result["minHeap"].push_back({
                {"deskId", entry.deskId},
                {"name", dName.str()},
                {"load", entry.load},
                {"isTop", i == 0}
            });
        }

        Customer mostUrgent;
        if (bank.peekNext(mostUrgent)) {
            result["nextCustomer"] = customerJson(mostUrgent);
        } else {
            result["nextCustomer"] = nullptr;
        }

        return jsonResponse(result, 200, "OK");
    }

    if (method == "GET" && path == "/api/history") {
        json history = json::array();
        for (const auto& customer : bank.history()) {
            history.push_back(customerJson(customer));
        }
        return jsonResponse({{"success", true}, {"history", history}}, 200, "OK");
    }

    if (method == "POST" && path == "/api/customer") {
        try {
            const json input = json::parse(body);
            if (!input.contains("name") || !input["name"].is_string()) {
                return errorResponse("Please enter customer name.");
            }
            std::string name = input["name"].get<std::string>();
            const auto start = name.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return errorResponse("Customer name cannot be empty.");
            }

            if (!input.contains("type") || !input["type"].is_string()) {
                return errorResponse("Invalid customer type. Must be NORMAL or VIP.");
            }
            std::string typeValue = input["type"].get<std::string>();
            std::transform(typeValue.begin(), typeValue.end(), typeValue.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

            CustomerType type;
            if (typeValue == "NORMAL") type = CustomerType::Normal;
            else if (typeValue == "VIP") type = CustomerType::Vip;
            else return errorResponse("Invalid customer type. Must be NORMAL or VIP.");

            int priority = 0;
            if (type == CustomerType::Vip) {
                if (!input.contains("priority")) {
                    return errorResponse("VIP priority is required (1=High, 2=Medium, 3=Low).");
                }
                if (input["priority"].is_number_integer()) {
                    priority = input["priority"].get<int>();
                } else if (input["priority"].is_string()) {
                    try {
                        priority = std::stoi(input["priority"].get<std::string>());
                    } catch (...) {
                        return errorResponse("Invalid priority. Priority must be an integer (1=High, 2=Medium, 3=Low).");
                    }
                } else {
                    return errorResponse("Invalid priority. Priority must be an integer (1=High, 2=Medium, 3=Low).");
                }
                if (priority < 1 || priority > 3) {
                    return errorResponse("VIP priority must be between 1 and 3 (1=High, 2=Medium, 3=Low).");
                }
            }

            const auto customer = bank.addCustomer(name, type, priority);
            return jsonResponse({
                {"success", true},
                {"token", customer.token},
                {"assignedDesk", customer.assignedDeskName},
                {"assignedDeskId", customer.assignedDeskId},
                {"customer", customerJson(customer)},
                {"message", "Customer assigned to " + customer.assignedDeskName + " successfully."}
            }, 201, "Created");
        } catch (const json::parse_error&) {
            return errorResponse("Malformed JSON request body.");
        } catch (const std::exception& error) {
            return errorResponse(error.what());
        }
    }

    if (method == "POST" && path == "/api/serve") {
        int deskId = 0;
        if (!body.empty()) {
            try {
                const json input = json::parse(body);
                if (input.contains("deskId")) {
                    if (!input["deskId"].is_number_integer()) {
                        return errorResponse("deskId must be an integer from 1 to 4.");
                    }
                    deskId = input["deskId"].get<int>();
                    if (deskId < 1 || deskId > 4) {
                        return errorResponse("deskId must be an integer from 1 to 4.");
                    }
                }
            } catch (const json::parse_error&) {
                return errorResponse("Malformed JSON request body.");
            }
        }

        Customer served;
        if (!bank.serveNext(served, deskId)) {
            if (deskId > 0) {
                std::ostringstream msg;
                msg << "No customers are currently waiting at Desk "
                    << (deskId < 10 ? "0" : "") << deskId << ".";
                return errorResponse(msg.str(), 409);
            }
            return errorResponse("No customers are currently waiting.", 409);
        }

        return jsonResponse({
            {"success", true},
            {"customer", customerJson(served)},
            {"servedDesk", served.assignedDeskName},
            {"servedDeskId", served.assignedDeskId},
            {"message", "Customer served successfully from " + served.assignedDeskName + "."}
        }, 200, "OK");
    }

    if (method == "POST" && path == "/api/reset") {
        bank.reset();
        return jsonResponse({
            {"success", true},
            {"message", "Simulator reset successfully across all 4 counters."}
        }, 200, "OK");
    }

    if (method == "POST" && path == "/api/demo") {
        // Enqueue standard demo customers through real C++ Min-Heap least-loaded allocation
        bank.addCustomer("Amit", CustomerType::Normal, 0);
        bank.addCustomer("Ravi", CustomerType::Normal, 0);
        bank.addCustomer("Kiran", CustomerType::Normal, 0);
        bank.addCustomer("Arjun", CustomerType::Normal, 0);
        bank.addCustomer("Vijay", CustomerType::Normal, 0);
        bank.addCustomer("Manoj", CustomerType::Normal, 0);
        bank.addCustomer("Suresh", CustomerType::Normal, 0);
        bank.addCustomer("Rahul", CustomerType::Normal, 0);

        bank.addCustomer("Priya", CustomerType::Vip, 1);
        bank.addCustomer("Neha", CustomerType::Vip, 2);
        bank.addCustomer("Karan", CustomerType::Vip, 3);
        bank.addCustomer("Rohit", CustomerType::Vip, 1);

        return jsonResponse({
            {"success", true},
            {"message", "12 demo customers allocated to counters using C++ Min-Heap."}
        }, 200, "OK");
    }

    return errorResponse("API endpoint not found.", 404);
}
}

int main() {
    WSADATA windowsSockets;
    if (WSAStartup(MAKEWORD(2, 2), &windowsSockets) != 0) {
        std::cerr << "Unable to initialize Windows sockets.\n";
        return 1;
    }

    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        std::cerr << "Unable to create API socket.\n";
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(8080);
    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR ||
        listen(listener, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Unable to start API on port 8080. Check if another process is using port 8080.\n";
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    Bank bank;
    std::cout << "====================================================\n";
    std::cout << " Banking Queue Simulator C++ API Server Online\n";
    std::cout << " Phase 2: Smart Counter Allocation (Min-Heap Enabled)\n";
    std::cout << " Listening on: http://localhost:8080\n";
    std::cout << " Endpoints: /api/queue, /api/desks, /api/customer,\n";
    std::cout << "            /api/serve, /api/history, /api/stats, /api/reset\n";
    std::cout << "====================================================\n";

    while (true) {
        SOCKET client = accept(listener, nullptr, nullptr);
        if (client == INVALID_SOCKET) break;

        DWORD timeoutMs = 2500;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));

        const std::string request = readRequest(client);
        if (!request.empty()) {
            const std::string response = handleRequest(bank, request);
            send(client, response.c_str(), static_cast<int>(response.size()), 0);
        }
        closesocket(client);
    }

    closesocket(listener);
    WSACleanup();
    return 0;
}
