# Railbooker Algorithm Analysis

This project uses a C++17 menu-based railway booking model with CSV-backed persistence and in-memory indexes while the program is running.

## Optimized Algorithms Used

| Feature | Data Structure / Algorithm | Time Complexity | Space Complexity |
|---|---|---:|---:|
| User login | `unordered_map<username, UserAccount>` plus salted deterministic hash | Average `O(1)` | `O(U)` users |
| Admin login | `unordered_map<username, AdminAccount>` plus salted deterministic hash | Average `O(1)` | `O(A)` admins |
| PNR lookup | `unordered_map<pnr, Booking>` | Average `O(1)` | `O(B)` bookings |
| Public PNR status | Direct PNR lookup plus passenger list print | Average `O(1 + k)` | `O(1)` extra |
| Passenger lookup | `unordered_map<passengerId, Passenger>` | Average `O(1)` | `O(P)` passengers |
| PNR passenger listing | `map<pnr, vector<passengerId>>` | `O(k)` for `k` passengers on a PNR | `O(P)` index |
| Train route search | Scan active trains and check ordered stop list | `O(T * S)` | `O(1)` extra per search |
| Class availability | Direct inventory lookup by `train|class` key | `O(1)` to `O(log C)` depending on map lookup | `O(I)` inventory rows |
| Confirmed seat allocation | Ordered `set<int>` of available seats, using `lower_bound` for berth range | `O(log S)` | `O(S)` available seats |
| RAC allocation | Append passenger id to per-train/per-class `deque` | `O(1)` | `O(R)` RAC queue |
| Waitlist allocation | Append passenger id to per-train/per-class `deque` | `O(1)` | `O(W)` waitlist queue |
| Confirmed cancellation | Return seat to ordered set, promote first RAC | `O(log S + 1)` typical | `O(1)` extra |
| RAC cancellation | Remove passenger from RAC queue and renumber | `O(R)` | `O(1)` extra |
| Waitlist cancellation | Remove passenger from WL queue and renumber | `O(W)` | `O(1)` extra |
| RAC to confirmed promotion | Pop first valid RAC passenger, assign freed seat | `O(log S)` | `O(1)` extra |
| WL to RAC promotion | Pop first valid WL passenger, append to RAC | `O(1)` typical | `O(1)` extra |
| Booking history | Scan bookings for username | `O(B)` | `O(M)` matching bookings |
| User train/date view | Sort active train numbers and print stops/classes | `O(T log T + T * (S + C))` | `O(T)` train list |
| Train-number route/status search | Direct train lookup plus route scan | `O(S + C)` | `O(1)` extra |
| Admin booking report | Sort bookings by booking time | `O(B log B)` | `O(B)` |
| Waitlist prediction | Binary search on fixed historical curve | `O(log H)`, effectively constant | `O(H)`, fixed |
| CSV load at startup | Read each dataset once | `O(N)` total records | `O(N)` runtime state |
| CSV save after mutation | Rewrite datasets once per completed operation | `O(N)` total records | `O(N)` rows while writing |

Symbols:

- `U`: users
- `A`: admins
- `T`: trains
- `S`: stops per train or seats per class, depending on row
- `C`: class count per train
- `B`: bookings
- `P`: passengers
- `I`: inventory rows
- `R`: RAC passengers for one train/class
- `W`: waitlisted passengers for one train/class
- `H`: waitlist prediction history points
- `N`: total stored CSV records

## Why This Avoids Lag

- PNR and login checks are hash lookups, not full scans.
- Seat, RAC, and WL state is stored per train and class, so one train's cancellation does not touch unrelated passengers.
- Confirmed seat allocation uses an ordered set and `lower_bound`, avoiding repeated sorting.
- CSV files are loaded once at startup and saved after completed actions, not repeatedly inside passenger loops.
- Booking and cancellation work on small train/class queues instead of one global passenger list.

## Worse Versions That Make Booking Systems Lag

| Bad Approach | Why It Lags | Typical Complexity |
|---|---|---:|
| Store users in a plain list and scan for login | Every login checks every user | `O(U)` per login |
| Store PNR records in a vector and scan | Every PNR query becomes slower as bookings grow | `O(B)` per query |
| Use one global waitlist for all trains/classes | Cancellation on one train scans unrelated waitlisted passengers | `O(P)` per promotion |
| Re-sort full waitlist after every booking/cancel | Sorting repeats work that FIFO queues already preserve | `O(W log W)` per update |
| Recompute availability by scanning all passengers | Availability screen becomes slower with every historical booking | `O(P)` per class display |
| Save CSV files inside each passenger allocation loop | Six passengers can trigger six full database rewrites | `O(kN)` instead of `O(N)` |
| Use unordered text parsing for every menu action | Re-reading files on every screen causes disk I/O lag | `O(N)` per screen |
| Mix RAC/WL queues across classes | Promotions need filtering and can produce wrong upgrades | `O(P)` and incorrect behavior |
| Allocate seats by scanning from seat 1 every time | High occupancy makes every booking slower | Worst `O(S)` per seat |
| Keep cancelled passengers inside active queues forever | Promotion loops waste time skipping dead records | Can degrade toward `O(W)` repeatedly |

## Notes

The password hash is intentionally simple and deterministic for an educational C++ project. It prevents plain-text password storage in CSV, but it is not production-grade security. Real systems should use a slow password hashing algorithm such as Argon2, bcrypt, or scrypt with unique per-user salts.
