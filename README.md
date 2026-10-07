# Banking Queue Simulator (Smart Counter Allocation)

A real-time, browser-based data structures project for managing normal and VIP bank customers. The user interface is vanilla HTML/CSS/JavaScript, and every queue operation is executed by a C++ backend using real Standard Template Library (STL) data structures.

## Objective

The simulator models four bank service counters:

* A C++ min-heap assigns each new customer to the least-loaded desk.
* Every desk owns a `std::queue<Customer>` for normal customers and a
  `std::priority_queue<Customer, std::vector<Customer>, VipCompare>` for VIPs.
* Normal customers are served FIFO and VIP customers are served by priority
  within their assigned desk.
* **VIP priority levels**: `1 = HIGH`, `2 = MEDIUM`, `3 = LOW`. Equal priorities strictly preserve arrival order (FIFO tie-breaking).
* **Service Algorithm**:
  1. If VIP queue is not empty, serve the highest-priority VIP (`vipQueue.top()`).
  2. Else if normal queue is not empty, serve the front normal customer (`normalQueue.front()`).
  3. Else, return "No customers waiting".

---

## Architecture

```text
Browser Frontend (http://localhost:5500)
        │
        │ JSON over HTTP (Port 8080) / CORS
        ▼
C++ Winsock HTTP API (backend/src/server.cpp)
        │
        ▼
Bank Class (service desk, history deque, statistics)
        │
        ▼
QueueManager
   └── Min-Heap Counter Allocation
       └── Desk 01..04
           ├── std::queue<Customer> (Normal FIFO)
           └── std::priority_queue<Customer, ...> (VIP priority)
```

---

## Quick Start (Windows)

You can launch both servers with one double-click each:

### Option 1: One-Click Launchers (Easiest)
1. **Start Backend**: Double-click `start_backend.bat` (Starts C++ server at `http://localhost:8080`).
2. **Start Frontend**: Double-click `start_frontend.bat` (Starts Python server at `http://localhost:5500`).
3. Open `http://localhost:5500` in your browser.

---

### Option 2: Terminal Commands

**Terminal 1 — Backend (C++ API Server on Port 8080):**
```powershell
cd "backend"
# Run pre-compiled server:
.\bank_server.exe

# Or recompile using MinGW g++:
g++ -std=c++14 -O2 -I include -I third_party src/server.cpp src/Desk.cpp src/Bank.cpp src/QueueManager.cpp -lws2_32 -o bank_server.exe
.\bank_server.exe
```

**Terminal 2 — Frontend (HTTP Server on Port 5500):**
```powershell
cd "frontend"
python -m http.server 5500
```

Browser URL: **`http://localhost:5500`**

---

## API Endpoints

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/health` or `/` | Health check & server status |
| `GET` | `/api/queue` | All four desk states, queues, heap state, totals, and global next customer |
| `GET` | `/api/desks` | Desk/counter states (same source of truth as `/api/queue`) |
| `GET` | `/api/stats` | Waiting counts across all desks and total served count |
| `GET` | `/api/history` | Audit trail of recently served customers with timestamps |
| `POST` | `/api/customer` | Add customer `{ "name": "...", "type": "NORMAL" \| "VIP", "priority": 1 }` |
| `POST` | `/api/serve` | Serve a desk (`{ "deskId": 2 }`), or the most urgent desk when omitted |
| `POST` | `/api/demo` | Populate standard demo customers through real C++ queues |
| `POST` | `/api/reset` | Clear all queues, reset tokens, history, and statistics |

---

## Running C++ Unit Tests

Run the test script:
```powershell
.\run_tests.bat
```
Or directly:
```powershell
cd backend
g++ -std=c++14 -O2 -I include tests/queue_manager_tests.cpp src/Desk.cpp src/QueueManager.cpp src/Bank.cpp -o tests/queue_tests.exe
.\tests\queue_tests.exe
```

Unit tests verify:
* `std::queue<Customer>` FIFO insertion (`push`), inspection (`front`), removal (`pop`).
* `std::priority_queue<Customer>` heap insertion (`push`), inspection (`top`), removal (`pop`), priority ordering (1 > 2 > 3), and arrival-order tie-breaking.
* Service algorithm (VIP prioritized over Normal, empty queue error handling).
* Bank history deque and formatted timestamps (`%I:%M %p`).
* Token generation (`N-001...`, `V-001...`) and reset behavior.
* Prompt Requirement 22 End-to-End Test Scenario.
