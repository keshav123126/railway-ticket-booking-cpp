#include "common.h"

struct ReceiptLine {
    string passengerName;
    string previousAllocation;
    double fare;
    double refund;
};

static double calculateRefund(double fare, const string& journeyDate) {
    tm journeyTm = {};
    if (!parseDateToTm(journeyDate, journeyTm)) return fare * 0.25;
    time_t journey = mktime(&journeyTm);
    double hoursToJourney = difftime(journey, time(nullptr)) / 3600.0;
    if (hoursToJourney > 48.0) return fare * 0.90;
    if (hoursToJourney > 12.0) return fare * 0.50;
    return fare * 0.25;
}

static string allocationText(const Passenger& p) {
    if (p.status == Status::CNF) return "Seat " + to_string(p.seatNumber);
    if (p.status == Status::RAC) return "RAC " + to_string(p.racPosition);
    if (p.status == Status::WL) return "WL " + to_string(p.wlPosition);
    return "Cancelled";
}

static void removeFromQueue(deque<string>& q, const string& passengerId) {
    q.erase(remove(q.begin(), q.end(), passengerId), q.end());
}

static void renumberQueue(SystemState& state, Inventory& inv, bool racQueue) {
    deque<string>& q = racQueue ? inv.racQueue : inv.wlQueue;
    int pos = 1;
    for (const string& id : q) {
        if (!state.passengers.count(id)) continue;
        Passenger& p = state.passengers[id];
        if (racQueue && p.status == Status::RAC) p.racPosition = pos++;
        if (!racQueue && p.status == Status::WL) p.wlPosition = pos++;
    }
}

void refreshBookingStatus(SystemState& state, const string& pnr) {
    if (!state.bookings.count(pnr)) return;
    auto idsIt = state.passengersByPnr.find(pnr);
    if (idsIt == state.passengersByPnr.end() || idsIt->second.empty()) {
        state.bookings[pnr].status = Status::CANCELLED;
        return;
    }

    bool hasActive = false;
    bool hasRac = false;
    bool hasWl = false;
    for (const string& id : idsIt->second) {
        if (!state.passengers.count(id)) continue;
        Status s = state.passengers[id].status;
        if (s != Status::CANCELLED) hasActive = true;
        if (s == Status::WL) hasWl = true;
        if (s == Status::RAC) hasRac = true;
    }

    if (!hasActive) state.bookings[pnr].status = Status::CANCELLED;
    else if (hasWl) state.bookings[pnr].status = Status::WL;
    else if (hasRac) state.bookings[pnr].status = Status::RAC;
    else state.bookings[pnr].status = Status::CNF;
}

static bool cancelPassenger(SystemState& state, const string& passengerId, ReceiptLine& line) {
    if (!state.passengers.count(passengerId)) return false;
    Passenger& p = state.passengers[passengerId];
    if (p.status == Status::CANCELLED) return false;

    string key = inventoryKey(p.trainNumber, p.classCode);
    if (!state.inventories.count(key)) return false;
    Inventory& inv = state.inventories[key];

    line.passengerName = p.name;
    line.previousAllocation = allocationText(p);
    line.fare = p.farePaid;
    line.refund = calculateRefund(p.farePaid, p.journeyDate);

    Status oldStatus = p.status;
    if (oldStatus == Status::CNF) {
        if (p.seatNumber > 0) inv.availableSeats.insert(p.seatNumber);
        p.seatNumber = -1;
    } else if (oldStatus == Status::RAC) {
        removeFromQueue(inv.racQueue, passengerId);
        renumberQueue(state, inv, true);
    } else if (oldStatus == Status::WL) {
        removeFromQueue(inv.wlQueue, passengerId);
        renumberQueue(state, inv, false);
    }

    p.status = Status::CANCELLED;
    p.racPosition = 0;
    p.wlPosition = 0;

    if (oldStatus == Status::CNF) promoteRACToCNF(state, p.trainNumber, p.classCode);
    else if (oldStatus == Status::RAC) promoteWaitlistToRAC(state, p.trainNumber, p.classCode);

    refreshBookingStatus(state, p.pnr);
    return true;
}

static void printReceipt(const Booking& booking, const vector<ReceiptLine>& lines) {
    double totalFare = 0.0;
    double totalRefund = 0.0;
    for (const ReceiptLine& line : lines) {
        totalFare += line.fare;
        totalRefund += line.refund;
    }

    cout << colorize("\n============== CANCELLATION RECEIPT ==============\n", UiColor::BOLD_CYAN);
    cout << "PNR          : " << colorize(booking.pnr, UiColor::BOLD_YELLOW) << "\n";
    cout << "Username     : " << booking.username << "\n";
    cout << "Cancelled At : " << nowString() << "\n";
    cout << "Train        : " << colorize(booking.trainNumber + " - " + booking.trainName, UiColor::BOLD_WHITE) << "\n";
    cout << "Route        : " << colorize(booking.source + " -> " + booking.destination, UiColor::BOLD_GREEN) << "\n";
    cout << "Class        : " << colorize(classLabel(booking.classCode), UiColor::BOLD_CYAN) << "\n";
    cout << "\nCancelled Passengers\n";
    cout << left << setw(4) << "No." << setw(24) << "Name" << setw(16) << "Old Status"
         << setw(12) << "Fare" << setw(12) << "Refund" << "\n";
    cout << string(68, '-') << "\n";
    for (size_t i = 0; i < lines.size(); i++) {
        cout << left << setw(4) << (i + 1)
             << setw(24) << lines[i].passengerName.substr(0, 23)
             << setw(16) << lines[i].previousAllocation
             << fixed << setprecision(2)
             << setw(12) << lines[i].fare
             << setw(12) << lines[i].refund << "\n";
    }
    cout << string(68, '-') << "\n";
    cout << fixed << setprecision(2);
    ostringstream fareLine;
    ostringstream refundLine;
    fareLine << fixed << setprecision(2) << "Total Fare Cancelled : Rs. " << totalFare << "\n";
    refundLine << fixed << setprecision(2) << "Total Refund         : Rs. " << totalRefund << "\n";
    cout << colorize(fareLine.str(), UiColor::BOLD_YELLOW);
    cout << colorize(refundLine.str(), UiColor::BOLD_GREEN);
    cout << colorize("==================================================\n", UiColor::BOLD_CYAN);
}

void cancellationMenu(SystemState& state, const string& username) {
    clearScreen();
    cout << colorize("=== CANCEL TICKET ===\n\n", UiColor::BOLD_CYAN);
    string pnr = readLineRequired("Enter PNR: ");
    if (!state.bookings.count(pnr)) {
        cout << colorize("PNR not found.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }
    Booking bookingBefore = state.bookings[pnr];
    if (bookingBefore.username != username) {
        cout << colorize("This PNR belongs to another user.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }

    vector<string> activeIds;
    auto idsIt = state.passengersByPnr.find(pnr);
    if (idsIt != state.passengersByPnr.end()) {
        for (const string& id : idsIt->second) {
            if (state.passengers.count(id) && state.passengers[id].status != Status::CANCELLED) {
                activeIds.push_back(id);
            }
        }
    }
    if (activeIds.empty()) {
        cout << colorize("All passengers under this PNR are already cancelled.\n", UiColor::BOLD_YELLOW);
        pauseScreen();
        return;
    }

    cout << "\nPassengers\n";
    cout << left << setw(4) << "No." << setw(24) << "Name" << setw(8) << "Age"
         << setw(14) << "Status" << setw(12) << "Allocation" << "\n";
    cout << string(62, '-') << "\n";
    for (size_t i = 0; i < activeIds.size(); i++) {
        const Passenger& p = state.passengers[activeIds[i]];
        cout << left << setw(4) << (i + 1)
             << setw(24) << p.name.substr(0, 23)
             << setw(8) << p.age
             << statusColoredPadded(p.status, 14)
             << setw(12) << allocationText(p) << "\n";
    }

    cout << "\nEnter 0 to cancel full PNR, or choose one passenger.\n";
    int choice = readInt("Choice: ", 0, (int)activeIds.size());
    vector<string> toCancel;
    if (choice == 0) toCancel = activeIds;
    else toCancel.push_back(activeIds[choice - 1]);

    vector<ReceiptLine> receiptLines;
    for (const string& id : toCancel) {
        ReceiptLine line;
        if (cancelPassenger(state, id, line)) receiptLines.push_back(line);
    }

    if (receiptLines.empty()) {
        cout << colorize("No passengers were cancelled.\n", UiColor::BOLD_YELLOW);
        pauseScreen();
        return;
    }

    double totalRefund = 0.0;
    for (const ReceiptLine& line : receiptLines) totalRefund += line.refund;
    state.transactions.push_back({
        nowString(), pnr, username, "CANCELLATION_REFUND", totalRefund,
        "Refund for " + to_string(receiptLines.size()) + " cancelled passenger(s)"
    });
    addLog(state, username, "BOOKING_CANCELLED", "PNR " + pnr);
    saveDatabase(state);

    printReceipt(bookingBefore, receiptLines);
    pauseScreen();
}
