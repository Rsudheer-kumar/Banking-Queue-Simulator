# Architecture

## Request flow

The frontend never owns a customer list. It sends JSON requests to the C++ API and renders the returned state:

```text
GET /api/queue
        |
        v
server.cpp -> Bank -> QueueManager
                         |          |
                 normalQueue_  vipQueue_
```

`Bank` owns service history and the served counter. `QueueManager` owns the actual data structures. `server.cpp` is limited to HTTP validation, serialization, and routing.

## Service decision

```text
Serve Next
   |
   v
VIP queue?
 /       \
Yes       No
 |         |
Serve VIP  Normal queue?
             /       \
           Yes        No
            |          |
       Serve normal   Empty message
```

Token counters are kept separately for normal (`N-001`) and VIP (`V-001`) customers and reset with the simulator.
