#include "Bank.h"

#include <nlohmann/json.hpp>
#if defined(_WIN32) && !defined(WINAPI_FAMILY_PARTITION)
// Older MinGW headers do not define the Windows Store partition macros used
// by cpp-httplib's optional file-serving implementation.
#define WINAPI_FAMILY_PARTITION(...) 0
#endif
#include <httplib/httplib.h>
#include <algorithm>
#include <cstdlib>
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
    Bank bank;
    int port = 8080;
    if (const char* portValue = std::getenv("PORT")) {
        try {
            const long parsedPort = std::stol(portValue);
            if (parsedPort < 1 || parsedPort > 65535) {
                throw std::out_of_range("port out of range");
            }
            port = static_cast<int>(parsedPort);
        } catch (const std::exception&) {
            std::cerr << "PORT must be an integer between 1 and 65535.\n";
            return 1;
        }
    }

    httplib::Server server;
    auto handle = [&bank](const httplib::Request& request, httplib::Response& response) {
        std::ostringstream rawRequest;
        rawRequest << request.method << " " << request.target << " HTTP/1.1\r\n\r\n" << request.body;
        const std::string rawResponse = handleRequest(bank, rawRequest.str());

        const auto statusLineEnd = rawResponse.find("\r\n");
        const auto bodyStart = rawResponse.find("\r\n\r\n");
        if (statusLineEnd == std::string::npos || bodyStart == std::string::npos) {
            response.status = 500;
            response.set_content(R"({"success":false,"message":"Malformed server response."})",
                                 "application/json; charset=utf-8");
            return;
        }

        std::istringstream statusLine(rawResponse.substr(0, statusLineEnd));
        std::string httpVersion;
        statusLine >> httpVersion >> response.status;
        response.set_content(rawResponse.substr(bodyStart + 4), "application/json; charset=utf-8");
        response.set_header("Access-Control-Allow-Origin", "*");
        response.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        response.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    };
    server.Get(R"(.*)", handle);
    server.Post(R"(.*)", handle);
    server.Options(R"(.*)", handle);

    std::cout << "====================================================\n";
    std::cout << " Banking Queue Simulator C++ API Server Online\n";
    std::cout << " Phase 2: Smart Counter Allocation (Min-Heap Enabled)\n";
    std::cout << " Listening on: http://0.0.0.0:" << port << "\n";
    std::cout << " Endpoints: /api/queue, /api/desks, /api/customer,\n";
    std::cout << "            /api/serve, /api/history, /api/stats, /api/reset\n";
    std::cout << "====================================================\n";

    if (!server.listen("0.0.0.0", port)) {
        std::cerr << "Unable to start API on port " << port
                  << ". Check if another process is using it.\n";
        return 1;
    }
    return 0;
}
