#include "common.h"

static string allocationText(const Passenger& p) {
    if (p.status == Status::CNF) return "Seat " + to_string(p.seatNumber);
    if (p.status == Status::RAC) return "RAC " + to_string(p.racPosition);
    if (p.status == Status::WL) return "WL " + to_string(p.wlPosition);
    return "Cancelled";
}

static void printBookingDetails(const SystemState& state, const Booking& b) {
    cout << colorize("\n================ PNR STATUS ================\n", UiColor::BOLD_CYAN);
    cout << "PNR          : " << colorize(b.pnr, UiColor::BOLD_YELLOW) << "\n";
    cout << "Username     : " << b.username << "\n";
    cout << "Train        : " << colorize(b.trainNumber + " - " + b.trainName, UiColor::BOLD_WHITE) << "\n";
    cout << "Route        : " << colorize(b.source + " -> " + b.destination, UiColor::BOLD_GREEN) << "\n";
    cout << "Date         : " << colorize(b.journeyDate, UiColor::BOLD_YELLOW) << "\n";
    cout << "Class        : " << colorize(classLabel(b.classCode), UiColor::BOLD_CYAN) << "\n";
    cout << "Booked At    : " << b.bookingTime << "\n";
    cout << "Status       : " << statusColored(b.status) << "\n";
    cout << fixed << setprecision(2) << "Total Fare   : Rs. " << b.totalFare << "\n";
    cout << "\nPassengers\n";
    cout << left << setw(4) << "No." << setw(24) << "Name" << setw(6) << "Age"
         << setw(14) << "Status" << setw(12) << "Allocation" << setw(10) << "Pref" << "\n";
    cout << string(72, '-') << "\n";
    auto idsIt = state.passengersByPnr.find(b.pnr);
    if (idsIt != state.passengersByPnr.end()) {
        int i = 1;
        for (const string& id : idsIt->second) {
            if (!state.passengers.count(id)) continue;
            const Passenger& p = state.passengers.at(id);
            cout << left << setw(4) << i++
                 << setw(24) << p.name.substr(0, 23)
                 << setw(6) << p.age
                 << statusColoredPadded(p.status, 14)
                 << setw(12) << allocationText(p)
                 << setw(10) << berthPrefToString(p.preference) << "\n";
        }
    }
    cout << colorize("============================================\n", UiColor::BOLD_CYAN);
}
void publicPnrStatusMenu(SystemState& state) {
    clearScreen();
    cout << colorize("=== PUBLIC PNR STATUS ===\n\n", UiColor::BOLD_CYAN);
    cout << colorize("Login is not required for this PNR status check.\n\n", UiColor::DIM);
    string pnr = readLineRequired("Enter PNR: ");
    if (!state.bookings.count(pnr)) {
        cout << colorize("PNR not found.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }
    printBookingDetails(state, state.bookings[pnr]);
    pauseScreen();
}
static string minutesText(long long seconds) {
    if (seconds < 0) seconds = -seconds;
    long long minutes = seconds / 60;
    long long hours = minutes / 60;
    long long days = hours / 24;
    hours %= 24;
    minutes %= 60;
    ostringstream out;
    if (days > 0) out << days << "d ";
    if (hours > 0 || days > 0) out << hours << "h ";
    out << minutes << "m";
    return out.str();
}
static string trainRunningStatus(const vector<Stop>& stops) {
    if (stops.empty()) return colorize("Schedule unavailable", UiColor::BOLD_RED);
    time_t now = time(nullptr);
    time_t firstDeparture = 0;
    time_t lastArrival = 0;
    if (!parseDateTimeToTime(stops.front().departure, firstDeparture) ||
        !parseDateTimeToTime(stops.back().arrival, lastArrival)) {
        return colorize("Schedule time format unavailable", UiColor::BOLD_RED);
    }
    if (now < firstDeparture) {
        return colorize("Not started", UiColor::BOLD_YELLOW) + " | Starts in " + minutesText(firstDeparture - now);
    }
    if (now > lastArrival) {
        return colorize("Journey completed", UiColor::BOLD_GREEN) + " | Completed " + minutesText(now - lastArrival) + " ago";
    }
    for (size_t i = 0; i < stops.size(); i++) {
        time_t arrival = 0;
        time_t departure = 0;
        if (parseDateTimeToTime(stops[i].arrival, arrival) &&
            parseDateTimeToTime(stops[i].departure, departure) &&
            now >= arrival && now <= departure) {
            return colorize("At station", UiColor::BOLD_GREEN) + " | " + stops[i].station +
                   " | Departs in " + minutesText(departure - now);
        }
        if (i + 1 < stops.size()) {
            time_t fromDeparture = 0;
            time_t nextArrival = 0;
            if (parseDateTimeToTime(stops[i].departure, fromDeparture) &&
                parseDateTimeToTime(stops[i + 1].arrival, nextArrival) &&
                now > fromDeparture && now < nextArrival) {
                double progress = 0.0;
                if (nextArrival > fromDeparture) {
                    progress = 100.0 * difftime(now, fromDeparture) / difftime(nextArrival, fromDeparture);
                }
                ostringstream out;
                out << colorize("Running", UiColor::BOLD_GREEN) << " | "
                    << stops[i].station << " -> " << stops[i + 1].station
                    << " | ETA " << stops[i + 1].arrival
                    << " | " << fixed << setprecision(0) << progress << "% of leg covered";
                return out.str();
            }
        }
    }
    return colorize("Running status cannot be estimated from saved schedule", UiColor::BOLD_YELLOW);
}
void trainNumberSearchMenu(SystemState& state) {
    clearScreen();
    cout << colorize("=== TRAIN NUMBER / ROUTE SEARCH ===\n\n", UiColor::BOLD_CYAN);
    string trainNumber = readLineRequired("Enter train number: ");
    if (!state.trains.count(trainNumber)) {
        cout << colorize("Train number not found.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }
    const Train& train = state.trains[trainNumber];
    auto stopsIt = state.stopsByTrain.find(trainNumber);
    static const vector<Stop> noStops;
    const vector<Stop>& stops = (stopsIt == state.stopsByTrain.end()) ? noStops : stopsIt->second;
    cout << "\n" << colorize(train.trainNumber + " - " + train.trainName, UiColor::BOLD_WHITE) << "\n";
    cout << "Status       : " << (train.active ? colorize("ACTIVE", UiColor::BOLD_GREEN) : colorize("INACTIVE", UiColor::BOLD_RED)) << "\n";
    if (!stops.empty()) {
        cout << "Route        : " << colorize(stops.front().station + " -> " + stops.back().station, UiColor::BOLD_GREEN) << "\n";
        cout << "Journey Time : " << colorize(stops.front().departure + " to " + stops.back().arrival, UiColor::BOLD_YELLOW) << "\n";
    }
    cout << "Live Status  : " << trainRunningStatus(stops) << "\n";
    cout << colorize("Note         : This is a timetable-based estimate, not GPS/live railway data.\n", UiColor::DIM);
    cout << "\nClass Availability\n";
    cout << left << setw(18) << "Class" << setw(12) << "CNF Free"
         << setw(10) << "RAC Free" << setw(10) << "WL Free" << setw(12) << "Fare" << "\n";
    cout << string(62, '-') << "\n";
    for (const string& cls : classesForTrain(state, trainNumber)) {
        const Inventory& inv = state.inventories.at(inventoryKey(trainNumber, cls));
        cout << left << setw(18) << classLabel(cls).substr(0, 17)
             << setw(12) << inv.availableSeats.size()
             << setw(10) << max(0, inv.racLimit - (int)inv.racQueue.size())
             << setw(10) << max(0, inv.wlLimit - (int)inv.wlQueue.size())
             << fixed << setprecision(2) << setw(12) << fareForClass(train.baseFare, cls) << "\n";
    }
    cout << "\nRoute Schedule\n";
    if (stops.empty()) {
        cout << colorize("No route schedule saved for this train.\n", UiColor::BOLD_YELLOW);
    } else {
        cout << left << setw(4) << "No." << setw(24) << "Station"
             << setw(18) << "Arrival" << setw(18) << "Departure" << "\n";
        cout << string(64, '-') << "\n";
        for (const Stop& stop : stops) {
            cout << left << setw(4) << stop.sequence
                 << setw(24) << stop.station.substr(0, 23)
                 << setw(18) << stop.arrival
                 << setw(18) << stop.departure << "\n";
        }
    }
    pauseScreen();
}
void pnrStatusMenu(SystemState& state, const string& username) {
    clearScreen();
    cout << colorize("=== VIEW MY PNR STATUS ===\n\n", UiColor::BOLD_CYAN);
    string pnr = readLineRequired("Enter PNR: ");
    if (!state.bookings.count(pnr)) {
        cout << colorize("PNR not found.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }
    const Booking& b = state.bookings[pnr];
    if (b.username != username) {
        cout << colorize("This PNR belongs to another user.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }
    printBookingDetails(state, b);
    pauseScreen();
}
void bookingHistoryMenu(SystemState& state, const string& username) {
    clearScreen();
    cout << colorize("=== MY BOOKING HISTORY ===\n\n", UiColor::BOLD_CYAN);
    vector<Booking> rows;
    for (const auto& [pnr, b] : state.bookings) {
        if (b.username == username) rows.push_back(b);
    }
    sort(rows.begin(), rows.end(), [](const Booking& a, const Booking& b) {
        return a.bookingTime > b.bookingTime;
    });
    if (rows.empty()) {
        cout << colorize("No bookings found.\n", UiColor::BOLD_YELLOW);
        pauseScreen();
        return;
    }
    cout << left << setw(14) << "PNR" << setw(10) << "Train" << setw(18) << "Class"
         << setw(16) << "Date" << setw(14) << "Status" << setw(12) << "Fare" << "\n";
    cout << string(84, '-') << "\n";
    for (const Booking& b : rows) {
        cout << left << setw(14) << b.pnr
             << setw(10) << b.trainNumber
             << setw(18) << classLabel(b.classCode).substr(0, 17)
             << setw(16) << b.journeyDate
             << statusColoredPadded(b.status, 14)
             << fixed << setprecision(2) << setw(12) << b.totalFare << "\n";
    }
    pauseScreen();
}
void userViewTrains(SystemState& state) {
    clearScreen();
    cout << colorize("=== VIEW TRAINS & DATES ===\n\n", UiColor::BOLD_CYAN);
    if (state.trains.empty()) {
        cout << colorize("No trains registered.\n", UiColor::BOLD_YELLOW);
        pauseScreen();
        return;
    }
    vector<string> trainNumbers;
    for (const auto& [trainNo, train] : state.trains) {
        if (train.active) trainNumbers.push_back(trainNo);
    }
    sort(trainNumbers.begin(), trainNumbers.end());
    if (trainNumbers.empty()) {
        cout << colorize("No active trains available.\n", UiColor::BOLD_YELLOW);
        pauseScreen();
        return;
    }
    for (const string& trainNo : trainNumbers) {
        const Train& train = state.trains.at(trainNo);
        auto stopsIt = state.stopsByTrain.find(trainNo);
        static const vector<Stop> noStops;
        const vector<Stop>& stops = (stopsIt == state.stopsByTrain.end()) ? noStops : stopsIt->second;
        cout << colorize(train.trainNumber + " - " + train.trainName + "\n", UiColor::BOLD_WHITE);
        if (!stops.empty()) {
            const Stop& first = stops.front();
            const Stop& last = stops.back();
            cout << "Route: " << colorize(first.station + " -> " + last.station, UiColor::BOLD_GREEN) << "\n";
            cout << "Available Date/Time: " << colorize(first.departure + " to " + last.arrival, UiColor::BOLD_YELLOW) << "\n";
        }
        cout << "Class Availability\n";
        cout << left << setw(18) << "Class" << setw(12) << "CNF Free"
             << setw(10) << "RAC Free" << setw(10) << "WL Free" << setw(12) << "Fare" << "\n";
        cout << string(62, '-') << "\n";
        for (const string& cls : classesForTrain(state, trainNo)) {
            const Inventory& inv = state.inventories.at(inventoryKey(trainNo, cls));
            cout << left << setw(18) << classLabel(cls).substr(0, 17)
                 << setw(12) << inv.availableSeats.size()
                 << setw(10) << max(0, inv.racLimit - (int)inv.racQueue.size())
                 << setw(10) << max(0, inv.wlLimit - (int)inv.wlQueue.size())
                 << fixed << setprecision(2) << setw(12) << fareForClass(train.baseFare, cls) << "\n";
        }
        if (!stops.empty()) {
            cout << "Schedule\n";
            cout << left << setw(4) << "No." << setw(24) << "Station"
                 << setw(18) << "Arrival" << setw(18) << "Departure" << "\n";
            cout << string(64, '-') << "\n";
            for (const Stop& stop : stops) {
                cout << left << setw(4) << stop.sequence
                     << setw(24) << stop.station.substr(0, 23)
                     << setw(18) << stop.arrival
                     << setw(18) << stop.departure << "\n";
            }
        }
        cout << string(78, '=') << "\n\n";
    }
    pauseScreen();
}
void trainAvailabilityMenu(SystemState& state) {
    clearScreen();
    cout << colorize("=== TRAIN AVAILABILITY ===\n\n", UiColor::BOLD_CYAN);
    string source = toUpperStr(readLineRequired("From station: "));
    string destination = toUpperStr(readLineRequired("To station: "));
    string date;
    while (true) {
        date = readLineRequired("Journey date (DD-MM-YYYY): ");
        if (isValidFutureDate(date)) break;
        cout << colorize("Enter today's or a future date in DD-MM-YYYY format.\n", UiColor::BOLD_RED);
    }
    bool any = false;
    for (const auto& [trainNo, train] : state.trains) {
        if (!train.active) continue;
        int srcIdx = -1;
        int dstIdx = -1;
        if (!trainServesRoute(state, trainNo, source, destination, date, &srcIdx, &dstIdx)) continue;
        any = true;
        const vector<Stop>& stops = state.stopsByTrain[trainNo];
        cout << "\n" << colorize(train.trainNumber + " - " + train.trainName, UiColor::BOLD_WHITE) << "\n";
        cout << "Departure: " << colorize(stops[srcIdx].station + " " + stops[srcIdx].departure, UiColor::BOLD_GREEN)
             << " | Arrival: " << colorize(stops[dstIdx].station + " " + stops[dstIdx].arrival, UiColor::BOLD_YELLOW) << "\n";
        cout << left << setw(18) << "Class" << setw(12) << "CNF Free"
             << setw(10) << "RAC Free" << setw(10) << "WL Free" << setw(12) << "Fare" << "\n";
        cout << string(62, '-') << "\n";
        for (const string& cls : classesForTrain(state, trainNo)) {
            const Inventory& inv = state.inventories.at(inventoryKey(trainNo, cls));
            cout << left << setw(18) << classLabel(cls).substr(0, 17)
                 << setw(12) << inv.availableSeats.size()
                 << setw(10) << max(0, inv.racLimit - (int)inv.racQueue.size())
                 << setw(10) << max(0, inv.wlLimit - (int)inv.wlQueue.size())
                 << fixed << setprecision(2) << setw(12) << fareForClass(train.baseFare, cls) << "\n";
        }
    }
    if (!any) cout << colorize("No trains found for this route/date.\n", UiColor::BOLD_YELLOW);
    pauseScreen();
}
void adminViewTrains(SystemState& state) {
    clearScreen();
    cout << colorize("=== ADMIN: TRAINS ===\n\n", UiColor::BOLD_CYAN);
    if (state.trains.empty()) {
        cout << colorize("No trains registered.\n", UiColor::BOLD_YELLOW);
        pauseScreen();
        return;
    }
    for (const auto& [trainNo, train] : state.trains) {
        cout << colorize(train.trainNumber + " - " + train.trainName, UiColor::BOLD_WHITE)
             << " | Active: " << (train.active ? colorize("Yes", UiColor::BOLD_GREEN) : colorize("No", UiColor::BOLD_RED))
             << fixed << setprecision(2) << " | Base Fare: Rs. " << train.baseFare << "\n";
        cout << "Classes: ";
        vector<string> cls = classesForTrain(state, trainNo);
        for (size_t i = 0; i < cls.size(); i++) {
            if (i) cout << ", ";
            cout << classLabel(cls[i]);
        }
        cout << "\nStops:\n";
        for (const Stop& stop : state.stopsByTrain[trainNo]) {
            cout << "  " << right << setw(2) << stop.sequence << ". " << left << setw(22) << stop.station
                 << " Arr: " << stop.arrival << " Dep: " << stop.departure << "\n";
        }
        cout << string(92, '-') << "\n";
    }
    pauseScreen();
}
void adminViewBookings(SystemState& state) {
    clearScreen();
    cout << colorize("=== ADMIN: ALL BOOKINGS ===\n\n", UiColor::BOLD_CYAN);
    if (state.bookings.empty()) {
        cout << colorize("No bookings found.\n", UiColor::BOLD_YELLOW);
        pauseScreen();
        return;
    }
    vector<Booking> rows;
    for (const auto& [pnr, b] : state.bookings) rows.push_back(b);
    sort(rows.begin(), rows.end(), [](const Booking& a, const Booking& b) {
        return a.bookingTime > b.bookingTime;
    });
    cout << left << setw(14) << "PNR" << setw(16) << "User" << setw(10) << "Train"
         << setw(8) << "Class" << setw(14) << "Date" << setw(14) << "Status"
         << setw(12) << "Fare" << "\n";
    cout << string(88, '-') << "\n";
    for (const Booking& b : rows) {
        cout << left << setw(14) << b.pnr
             << setw(16) << b.username.substr(0, 15)
             << setw(10) << b.trainNumber
             << setw(8) << b.classCode
             << setw(14) << b.journeyDate
             << statusColoredPadded(b.status, 14)
             << fixed << setprecision(2) << setw(12) << b.totalFare << "\n";
    }
    pauseScreen();
}

void adminViewLogs(SystemState& state) {
    clearScreen();
    cout << colorize("=== ADMIN: SYSTEM LOGS ===\n\n", UiColor::BOLD_CYAN);
    if (state.logs.empty()) {
        cout << colorize("No logs found.\n", UiColor::BOLD_YELLOW);
        pauseScreen();
        return;
    }
    int start = max(0, (int)state.logs.size() - 100);
    cout << left << setw(22) << "Timestamp" << setw(16) << "Actor"
         << setw(24) << "Action" << "Details\n";
    cout << string(96, '-') << "\n";
    for (int i = start; i < (int)state.logs.size(); i++) {
        const LogEntry& log = state.logs[i];
        cout << left << setw(22) << log.timestamp
             << setw(16) << log.actor.substr(0, 15)
             << setw(24) << log.action.substr(0, 23)
             << log.details << "\n";
    }
    pauseScreen();
}