#include "common.h"

struct TrainMatch {
    string trainNumber;
    string trainName;
    string sourceDeparture;
    string destinationArrival;
};

static pair<int, int> berthBounds(BerthPref pref, int totalSeats) {
    int firstEnd = max(1, totalSeats / 3);
    int secondEnd = max(firstEnd, (2 * totalSeats) / 3);
    if (pref == BerthPref::LOWER) return {1, firstEnd};
    if (pref == BerthPref::MIDDLE) return {firstEnd + 1, secondEnd};
    if (pref == BerthPref::UPPER) return {secondEnd + 1, totalSeats};
    return {1, totalSeats};
}

static int allocateSeatOptimized(Inventory& inv, BerthPref pref) {
    if (inv.availableSeats.empty()) return -1;

    ClassInfo info = getClassInfo(inv.classCode);
    if (info.berthBased && pref != BerthPref::ANY) {
        auto [lo, hi] = berthBounds(pref, inv.totalSeats);
        auto it = inv.availableSeats.lower_bound(lo);
        if (it != inv.availableSeats.end() && *it <= hi) {
            int seat = *it;
            inv.availableSeats.erase(it);
            return seat;
        }
    }

    auto it = inv.availableSeats.begin();
    int seat = *it;
    inv.availableSeats.erase(it);
    return seat;
}

static int freeCapacity(const Inventory& inv) {
    int confirmed = (int)inv.availableSeats.size();
    int rac = max(0, inv.racLimit - (int)inv.racQueue.size());
    int wl = max(0, inv.wlLimit - (int)inv.wlQueue.size());
    return confirmed + rac + wl;
}

static Status summarizeBooking(const vector<Passenger>& passengers) {
    bool hasRac = false;
    bool hasWl = false;
    for (const Passenger& p : passengers) {
        if (p.status == Status::WL) hasWl = true;
        else if (p.status == Status::RAC) hasRac = true;
    }
    if (hasWl) return Status::WL;
    if (hasRac) return Status::RAC;
    return Status::CNF;
}

static void printTicket(const SystemState& state, const string& pnr) {
    auto bookingIt = state.bookings.find(pnr);
    if (bookingIt == state.bookings.end()) return;
    const Booking& b = bookingIt->second;

    cout << colorize("\n================== TICKET ==================\n", UiColor::BOLD_CYAN);
    cout << "PNR          : " << colorize(b.pnr, UiColor::BOLD_YELLOW) << "\n";
    cout << "Username     : " << b.username << "\n";
    cout << "Booked At    : " << b.bookingTime << "\n";
    cout << "Train        : " << colorize(b.trainNumber + " - " + b.trainName, UiColor::BOLD_WHITE) << "\n";
    cout << "Route        : " << colorize(b.source + " -> " + b.destination, UiColor::BOLD_GREEN) << "\n";
    cout << "Journey Date : " << colorize(b.journeyDate, UiColor::BOLD_YELLOW) << "\n";
    cout << "Class        : " << colorize(classLabel(b.classCode), UiColor::BOLD_CYAN) << "\n";
    cout << "Status       : " << statusColored(b.status) << "\n";
    ostringstream fareLine;
    fareLine << fixed << setprecision(2) << "Rs. " << b.totalFare;
    cout << "Total Fare   : " << colorize(fareLine.str(), UiColor::BOLD_YELLOW) << "\n";
    cout << "\nPassengers\n";
    cout << left << setw(4) << "No." << setw(22) << "Name" << setw(6) << "Age"
         << setw(12) << "Status" << setw(14) << "Seat/RAC/WL" << setw(10) << "Pref" << "\n";
    cout << string(68, '-') << "\n";

    auto idsIt = state.passengersByPnr.find(pnr);
    if (idsIt != state.passengersByPnr.end()) {
        int row = 1;
        for (const string& id : idsIt->second) {
            const Passenger& p = state.passengers.at(id);
            string allocation = "-";
            if (p.status == Status::CNF) allocation = "Seat " + to_string(p.seatNumber);
            else if (p.status == Status::RAC) allocation = "RAC " + to_string(p.racPosition);
            else if (p.status == Status::WL) allocation = "WL " + to_string(p.wlPosition);
            cout << left << setw(4) << row++
                 << setw(22) << p.name.substr(0, 21)
                 << setw(6) << p.age
                 << statusColoredPadded(p.status, 12)
                 << setw(14) << allocation
                 << setw(10) << berthPrefToString(p.preference) << "\n";
        }
    }
    cout << colorize("============================================\n", UiColor::BOLD_CYAN);
}

static vector<TrainMatch> searchTrains(SystemState& state, const string& source,
                                       const string& destination, const string& date) {
    vector<TrainMatch> matches;
    for (const auto& [trainNo, train] : state.trains) {
        if (!train.active) continue;
        int srcIdx = -1;
        int dstIdx = -1;
        if (!trainServesRoute(state, trainNo, source, destination, date, &srcIdx, &dstIdx)) continue;
        const vector<Stop>& stops = state.stopsByTrain[trainNo];
        matches.push_back({trainNo, train.trainName, stops[srcIdx].departure, stops[dstIdx].arrival});
    }
    sort(matches.begin(), matches.end(), [](const TrainMatch& a, const TrainMatch& b) {
        return a.sourceDeparture < b.sourceDeparture;
    });
    return matches;
}

void bookingMenu(SystemState& state, const string& username) {
    clearScreen();
    cout << colorize("=== BOOK TICKET ===\n\n", UiColor::BOLD_CYAN);

    string source = toUpperStr(readLineRequired("From station: "));
    string destination = toUpperStr(readLineRequired("To station: "));
    if (source == destination) {
        cout << colorize("Source and destination cannot be same.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }

    string journeyDate;
    while (true) {
        journeyDate = readLineRequired("Journey date (DD-MM-YYYY): ");
        if (isValidFutureDate(journeyDate)) break;
        cout << colorize("Enter today's or a future date in DD-MM-YYYY format.\n", UiColor::BOLD_RED);
    }

    vector<TrainMatch> matches = searchTrains(state, source, destination, journeyDate);
    if (matches.empty()) {
        cout << colorize("\nNo trains found for this route/date.\n", UiColor::BOLD_YELLOW);
        pauseScreen();
        return;
    }

    cout << "\nAvailable Trains\n";
    cout << left << setw(4) << "No." << setw(10) << "Train" << setw(24) << "Name"
         << setw(18) << "Departure" << setw(18) << "Arrival" << "\n";
    cout << string(74, '-') << "\n";
    for (size_t i = 0; i < matches.size(); i++) {
        cout << left << setw(4) << (i + 1)
             << setw(10) << matches[i].trainNumber
             << setw(24) << matches[i].trainName.substr(0, 23)
             << setw(18) << matches[i].sourceDeparture
             << setw(18) << matches[i].destinationArrival << "\n";
    }

    int trainChoice = readInt("\nSelect train: ", 1, (int)matches.size());
    TrainMatch selected = matches[trainChoice - 1];
    const Train& train = state.trains[selected.trainNumber];
    vector<string> classCodes = classesForTrain(state, selected.trainNumber);
    if (classCodes.empty()) {
        cout << colorize("No class inventory found for this train.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }

    cout << "\nAvailable Classes\n";
    cout << left << setw(4) << "No." << setw(18) << "Class" << setw(12) << "CNF Free"
         << setw(10) << "RAC Free" << setw(10) << "WL Free" << setw(12) << "Fare" << "\n";
    cout << string(66, '-') << "\n";
    for (size_t i = 0; i < classCodes.size(); i++) {
        const Inventory& inv = state.inventories[inventoryKey(selected.trainNumber, classCodes[i])];
        int racFree = max(0, inv.racLimit - (int)inv.racQueue.size());
        int wlFree = max(0, inv.wlLimit - (int)inv.wlQueue.size());
        cout << left << setw(4) << (i + 1)
             << setw(18) << classLabel(classCodes[i]).substr(0, 17)
             << setw(12) << inv.availableSeats.size()
             << setw(10) << racFree
             << setw(10) << wlFree
             << fixed << setprecision(2) << setw(12) << fareForClass(train.baseFare, classCodes[i]) << "\n";
    }

    int classChoice = readInt("\nSelect class: ", 1, (int)classCodes.size());
    string classCode = classCodes[classChoice - 1];
    Inventory& inv = state.inventories[inventoryKey(selected.trainNumber, classCode)];
    int passengerCount = readInt("Number of passengers (1-6): ", 1, 6);
    if (passengerCount > freeCapacity(inv)) {
        cout << colorize("Not enough CNF/RAC/WL capacity available for this class.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }

    vector<Passenger> newPassengers;
    double farePerPassenger = fareForClass(train.baseFare, classCode);
    string pnr = generatePNR(state);

    for (int i = 1; i <= passengerCount; i++) {
        cout << "\nPassenger " << i << "\n";
        Passenger p;
        p.passengerId = pnr + "-" + to_string(i);
        p.pnr = pnr;
        p.username = username;
        p.name = readLineRequired("Name: ");
        p.age = readInt("Age: ", 1, 120);
        if (getClassInfo(classCode).berthBased) {
            p.preference = stringToPref(readLineRequired("Berth preference (L/M/U/A): "));
        } else {
            p.preference = BerthPref::ANY;
        }
        p.classCode = classCode;
        p.trainNumber = selected.trainNumber;
        p.source = source;
        p.destination = destination;
        p.journeyDate = journeyDate;
        p.seatNumber = -1;
        p.racPosition = 0;
        p.wlPosition = 0;
        p.farePaid = farePerPassenger;

        if (!inv.availableSeats.empty()) {
            p.status = Status::CNF;
            p.seatNumber = allocateSeatOptimized(inv, p.preference);
        } else if ((int)inv.racQueue.size() < inv.racLimit) {
            p.status = Status::RAC;
            p.racPosition = (int)inv.racQueue.size() + 1;
            inv.racQueue.push_back(p.passengerId);
        } else {
            p.status = Status::WL;
            p.wlPosition = (int)inv.wlQueue.size() + 1;
            inv.wlQueue.push_back(p.passengerId);
        }
        newPassengers.push_back(p);
    }

    Booking booking;
    booking.pnr = pnr;
    booking.username = username;
    booking.trainNumber = selected.trainNumber;
    booking.trainName = selected.trainName;
    booking.source = source;
    booking.destination = destination;
    booking.journeyDate = journeyDate;
    booking.classCode = classCode;
    booking.bookingTime = nowString();
    booking.totalFare = farePerPassenger * passengerCount;
    booking.status = summarizeBooking(newPassengers);
    state.bookings[pnr] = booking;

    for (const Passenger& p : newPassengers) {
        state.passengers[p.passengerId] = p;
        state.passengersByPnr[pnr].push_back(p.passengerId);
    }

    state.transactions.push_back({
        nowString(), pnr, username, "BOOKING_PAYMENT", booking.totalFare,
        selected.trainNumber + " " + classCode + " ticket booking"
    });
    addLog(state, username, "BOOKING_CREATED", "PNR " + pnr + " for train " + selected.trainNumber);
    saveDatabase(state);

    printTicket(state, pnr);
    pauseScreen();
}
