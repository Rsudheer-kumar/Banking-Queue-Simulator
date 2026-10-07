# DSA and Viva Explanation

## Smart counter allocation

The simulator has exactly four service counters. Each `Desk` owns its own
`std::queue<Customer>` for normal customers and
`std::priority_queue<Customer, ...>` for VIP customers. A new customer is
assigned by the C++ `QueueManager`, never by JavaScript.

`deskHeap_` is a real min-heap:
`std::priority_queue<DeskEntry, std::vector<DeskEntry>, DeskCompare>`.
`DeskCompare` orders the smallest waiting workload first and uses the desk ID
as a deterministic tie-breaker. Because `std::priority_queue` does not support
in-place updates, every desk entry includes a version. A workload change
increments the desk version and pushes a new entry. `syncHeapTop()` removes
stale entries until the top entry matches the desk's current load and version.

For `D` desks, selecting the least-loaded desk is O(1) after stale entries are
discarded, and inserting the refreshed entry is O(log D). With four desks,
the heap remains small and predictable while still demonstrating the required
data structure.

## 1. What is a Queue?

A queue is a linear data structure where data is inserted at the rear and removed from the front.

## 2. What is FIFO?

FIFO means **First In, First Out**. The customer who arrives first is served first.

## 3. Why use a Queue for normal customers?

Normal service should be fair. `std::queue<Customer>` directly provides `push`, `front`, `pop`, `empty`, and `size` with FIFO behavior.

## 4. What is a Priority Queue?

A priority queue removes the element with the highest priority first instead of simply removing the oldest element.

## 5. Why use it for VIP customers?

VIP customers can have high, medium, or low service priority. `std::priority_queue` keeps the best candidate at `top()`, so the backend does not manually sort a vector.

## 6. Difference between Queue and Priority Queue

Queue ordering depends only on arrival order. Priority queue ordering depends on the comparator; arrival order is used only as a tie-breaker here.

## 7. What happens on Serve Next?

`Bank::serveNext` asks `QueueManager` for the next customer. `QueueManager` checks `vipQueue_` first. If it is not empty, it calls `top()` and `pop()`. Otherwise it calls `front()` and `pop()` on `normalQueue_`.

## 8. Queue complexity

Normal insertion, front access, and removal are O(1).

## 9. Priority queue complexity

VIP insertion and removal are O(log n). Accessing the top element is O(1).

## 10. Why C++?

C++ provides the Standard Template Library, including well-tested `queue` and `priority_queue` containers, while making the data structure implementation visible for study.

## 11. Where is DSA actually used?

It is used in `backend/src/QueueManager.cpp`. The members `normalQueue_` and `vipQueue_` are the live in-memory structures used by every add, display, and serve operation.

## 12. Why not use an array?

An array would require manually implementing removal, shifting, and priority ordering. That would obscure the intended data structure and can make operations slower or error-prone.

## 13. What if both queues are empty?

The API returns HTTP 409 with `No customers are currently waiting.` The frontend displays the error without crashing.

## 14. How are VIP priorities compared?

The custom `VipCompare` compares the numeric priority first: `1` beats `2`, and `2` beats `3`. If priorities match, the smaller arrival order wins.

## 15. How does the frontend communicate with C++?

`frontend/app.js` uses `fetch` to send JSON to `http://localhost:8080/api`. The C++ server returns JSON and enables CORS for the local frontend port.

## Viva questions

1. **Why multiple counters?** They prevent one counter from becoming overloaded.
2. **What does smart allocation solve?** It distributes waiting work evenly.
3. **Why a min-heap?** The least-loaded counter is always at the top.
4. **Why not scan an array?** A scan is O(D) and moves assignment logic out of
   the intended C++ data structure.
5. **What is a min-heap?** A heap whose smallest element is at the root.
6. **Complexity of finding the minimum?** O(1) at the top; heap insertion is
   O(log D).
7. **How are ties handled?** The lower desk ID wins.
8. **How is workload updated?** The desk changes, its version increments, and a
   fresh heap entry is pushed.
9. **How does the heap interact with Queue?** The heap picks a desk; that
   desk's normal queue stores the customer in FIFO order.
10. **How does it interact with Priority Queue?** The heap picks a desk; that
    desk's VIP priority queue chooses the customer to serve.
11. **Why does every desk need its own queue?** Counters operate independently.
12. **VIP joins Desk 03?** It is pushed into Desk 03's VIP priority queue.
13. **Customer served from Desk 02?** Desk 02 pops its VIP top or normal front,
    then publishes a refreshed heap entry.
14. **How do stale entries work?** Version/load checks discard old heap entries.
15. **Why is C++ the source of truth?** It owns assignment, ordering, serving,
    totals, and history; the browser only sends requests and renders responses.
