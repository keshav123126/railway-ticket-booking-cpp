#include "common.h"

static void cleanQueue(SystemState& state, deque<string>& q, Status expected) {
    deque<string> cleaned;
    for (const string& id : q) {
        if (state.passengers.count(id) && state.passengers[id].status == expected) cleaned.push_back(id);
    }
    q.swap(cleaned);
}

static void renumberRAC(SystemState& state, Inventory& inv) {
    int pos = 1;
    for (const string& id : inv.racQueue) {
        if (state.passengers.count(id) && state.passengers[id].status == Status::RAC)
            state.passengers[id].racPosition = pos++;
    }
}

static void renumberWL(SystemState& state, Inventory& inv) {
    int pos = 1;
    for (const string& id : inv.wlQueue) {
        if (state.passengers.count(id) && state.passengers[id].status == Status::WL)
            state.passengers[id].wlPosition = pos++;
    }
}

static bool promoteWaitlistDirectToCNF(SystemState& state, Inventory& inv) {
    cleanQueue(state, inv.wlQueue, Status::WL);
    if (inv.wlQueue.empty() || inv.availableSeats.empty()) return false;

    string passengerId = inv.wlQueue.front();
    inv.wlQueue.pop_front();
    Passenger& p = state.passengers[passengerId];

    int seat = *inv.availableSeats.begin();
    inv.availableSeats.erase(inv.availableSeats.begin());
    p.status = Status::CNF;
    p.seatNumber = seat;
    p.wlPosition = 0;
    p.racPosition = 0;

    renumberWL(state, inv);
    refreshBookingStatus(state, p.pnr);
    addLog(state, "SYSTEM", "WL_PROMOTED_TO_CNF", "PNR " + p.pnr + " passenger " + p.name);
    return true;
}

void promoteWaitlistToRAC(SystemState& state, const string& trainNumber, const string& classCode) {
    string key = inventoryKey(trainNumber, classCode);
    if (!state.inventories.count(key)) return;
    Inventory& inv = state.inventories[key];
    if (inv.racLimit <= 0) return;

    cleanQueue(state, inv.racQueue, Status::RAC);
    cleanQueue(state, inv.wlQueue, Status::WL);

    while ((int)inv.racQueue.size() < inv.racLimit && !inv.wlQueue.empty()) {
        string passengerId = inv.wlQueue.front();
        inv.wlQueue.pop_front();
        Passenger& p = state.passengers[passengerId];
        p.status = Status::RAC;
        p.racPosition = (int)inv.racQueue.size() + 1;
        p.wlPosition = 0;
        p.seatNumber = -1;
        inv.racQueue.push_back(passengerId);
        refreshBookingStatus(state, p.pnr);
        addLog(state, "SYSTEM", "WL_PROMOTED_TO_RAC", "PNR " + p.pnr + " passenger " + p.name);
    }

    renumberRAC(state, inv);
    renumberWL(state, inv);
}

void promoteRACToCNF(SystemState& state, const string& trainNumber, const string& classCode) {
    string key = inventoryKey(trainNumber, classCode);
    if (!state.inventories.count(key)) return;
    Inventory& inv = state.inventories[key];

    cleanQueue(state, inv.racQueue, Status::RAC);
    bool promoted = false;

    while (!inv.racQueue.empty() && !inv.availableSeats.empty()) {
        string passengerId = inv.racQueue.front();
        inv.racQueue.pop_front();
        Passenger& p = state.passengers[passengerId];

        int seat = *inv.availableSeats.begin();
        inv.availableSeats.erase(inv.availableSeats.begin());
        p.status = Status::CNF;
        p.seatNumber = seat;
        p.racPosition = 0;
        p.wlPosition = 0;
        refreshBookingStatus(state, p.pnr);
        addLog(state, "SYSTEM", "RAC_PROMOTED_TO_CNF", "PNR " + p.pnr + " passenger " + p.name);
        promoted = true;
        break;
    }

    renumberRAC(state, inv);

    if (promoted) {
        promoteWaitlistToRAC(state, trainNumber, classCode);
    } else if (inv.racLimit <= 0) {
        promoteWaitlistDirectToCNF(state, inv);
    }
}

void displayWaitlist(SystemState& state) {
    clearScreen();
    cout << "=== WAITLIST STATUS ===\n\n";
    bool any = false;
    cout << left << setw(10) << "Train" << setw(8) << "Class" << setw(6) << "WL#"
         << setw(14) << "PNR" << setw(22) << "Passenger" << setw(16) << "User" << "\n";
    cout << string(76, '-') << "\n";

    for (auto& [key, inv] : state.inventories) {
        cleanQueue(state, inv.wlQueue, Status::WL);
        renumberWL(state, inv);
        for (const string& id : inv.wlQueue) {
            if (!state.passengers.count(id)) continue;
            const Passenger& p = state.passengers[id];
            any = true;
            cout << left << setw(10) << p.trainNumber
                 << setw(8) << p.classCode
                 << setw(6) << p.wlPosition
                 << setw(14) << p.pnr
                 << setw(22) << p.name.substr(0, 21)
                 << setw(16) << p.username.substr(0, 15) << "\n";
        }
    }

    if (!any) cout << "No waitlisted passengers.\n";
    pauseScreen();
}
