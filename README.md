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
C++ cross-platform HTTP API using vendored cpp-httplib (backend/src/server.cpp)
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

# Or recompile using a C++17 compiler with cpp-httplib support:
g++ -std=c++17 -O2 -I include -I third_party src/server.cpp src/Desk.cpp src/Bank.cpp src/QueueManager.cpp -o bank_server.exe
.\bank_server.exe
```

The server binds to `0.0.0.0`. Set `PORT` to override the default `8080`:

```powershell
$env:PORT = "8080"
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

## Problem Statement

Banks need to serve normal and VIP customers efficiently while distributing work across multiple service counters. This simulator demonstrates FIFO queues, priority queues, and min-heap-based smart counter allocation in a working browser application.

## Installation and VS Code Setup

1. Install a C++ compiler such as MinGW-w64 and Python on Windows.
2. Open this project folder in VS Code.
3. Ensure `g++`, `python`, and (optionally) CMake are available in the integrated terminal.
4. No external database or API key is required.

## DSA Explanation

Normal customers use `std::queue<Customer>` and are served FIFO. VIP customers use a per-desk `std::priority_queue<Customer, ...>` ordered by priority and arrival sequence. Desk allocation uses `std::priority_queue<DeskEntry, ...>` with an inverted comparator to provide min-heap behavior, selecting the least-loaded desk with deterministic tie-breaking. See [docs/dsa-explanation.md](docs/dsa-explanation.md) for complexity details.

## Project Limitations

Queue, statistics, and recently served history data are stored in memory inside the C++ backend. Restarting the backend clears active data. The current HTTP server is intended for local demonstration and is not configured as a production-grade authenticated service.

## Deployment Notes

The frontend is static HTML, CSS, and vanilla JavaScript and can be served by any static web server. The C++ backend must run separately and be reachable by the frontend. Update the centralized API base URL in `frontend/app.js` when deploying the frontend and backend on different hosts. The backend binds to `0.0.0.0` and reads its port from `PORT`, falling back to `8080` for local development. The supplied batch files target Windows; Linux can build it with the same source command and no Winsock library:

```bash
g++ -std=c++17 -O2 -pthread -I include -I third_party \
  src/server.cpp src/Desk.cpp src/Bank.cpp src/QueueManager.cpp \
  -o bank_server
PORT=8080 ./bank_server
```

### Docker deployment

Build and run the Linux image:

```bash
docker build -t banking-queue-simulator .
docker run --rm -e PORT=8080 -p 8080:8080 banking-queue-simulator
```

The Docker build uses the vendored cpp-httplib and nlohmann/json headers, so no external C++ framework or database is required.
