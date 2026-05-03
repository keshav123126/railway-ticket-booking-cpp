#include "common.h"

static const string USERS_FILE = "users.csv";
static const string ADMINS_FILE = "admins.csv";
static const string TRAINS_FILE = "trains.csv";
static const string STOPS_FILE = "stops.csv";
static const string INVENTORY_FILE = "inventory.csv";
static const string BOOKINGS_FILE = "bookings.csv";
static const string PASSENGERS_FILE = "passengers.csv";
static const string LOGS_FILE = "logs.csv";
static const string TRANSACTIONS_FILE = "transactions.csv";

string trim(const string& s) {
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start])) start++;
    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1])) end--;
    return s.substr(start, end - start);
}

string toUpperStr(const string& s) {
    string out = s;
    for (char& c : out) c = (char)toupper((unsigned char)c);
    return out;
}

string nowString() {
    time_t now = time(nullptr);
    tm* local = localtime(&now);
    ostringstream out;
    out << put_time(local, "%d-%m-%Y %H:%M:%S");
    return out.str();
}

string dateOffsetString(int daysFromToday) {
    time_t now = time(nullptr) + (long long)daysFromToday * 24LL * 60LL * 60LL;
    tm* local = localtime(&now);
    ostringstream out;
    out << put_time(local, "%d-%m-%Y");
    return out.str();
}

void enableAnsiColors() {
    // Modern Windows Terminal, PowerShell, and most Unix terminals support ANSI colors.
    // Keep this dependency-free so the project stays simple to compile with C++17.
}

string colorize(const string& text, UiColor color) {
    string code;
    if (color == UiColor::DIM) code = "\033[2m";
    else if (color == UiColor::RED) code = "\033[31m";
    else if (color == UiColor::GREEN) code = "\033[32m";
    else if (color == UiColor::YELLOW) code = "\033[33m";
    else if (color == UiColor::BLUE) code = "\033[34m";
    else if (color == UiColor::MAGENTA) code = "\033[35m";
    else if (color == UiColor::CYAN) code = "\033[36m";
    else if (color == UiColor::WHITE) code = "\033[37m";
    else if (color == UiColor::BOLD_RED) code = "\033[1;31m";
    else if (color == UiColor::BOLD_GREEN) code = "\033[1;32m";
    else if (color == UiColor::BOLD_YELLOW) code = "\033[1;33m";
    else if (color == UiColor::BOLD_CYAN) code = "\033[1;36m";
    else if (color == UiColor::BOLD_WHITE) code = "\033[1;37m";
    else return text;
    return code + text + "\033[0m";
}

string statusColored(Status s) {
    if (s == Status::CNF) return colorize(statusToString(s), UiColor::BOLD_GREEN);
    if (s == Status::RAC) return colorize(statusToString(s), UiColor::BOLD_YELLOW);
    if (s == Status::WL) return colorize(statusToString(s), UiColor::YELLOW);
    return colorize(statusToString(s), UiColor::BOLD_RED);
}

string statusColoredPadded(Status s, int width) {
    string plain = statusToString(s);
    string padded = statusColored(s);
    if ((int)plain.size() < width) padded += string(width - plain.size(), ' ');
    return padded;
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pauseScreen() {
    cout << colorize("\nPress Enter to continue...", UiColor::DIM);
    string ignored;
    getline(cin, ignored);
}

int readInt(const string& prompt, int minValue, int maxValue) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        try {
            int value = stoi(trim(line));
            if (value >= minValue && value <= maxValue) return value;
        } catch (...) {
        }
        cout << colorize("Invalid input. Enter a number from " + to_string(minValue) + " to " + to_string(maxValue) + ".\n", UiColor::BOLD_RED);
    }
}

double readDouble(const string& prompt, double minValue, double maxValue) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        try {
            double value = stod(trim(line));
            if (value >= minValue && value <= maxValue) return value;
        } catch (...) {
        }
        cout << fixed << setprecision(2);
        ostringstream error;
        error << "Invalid input. Enter a value from " << minValue << " to " << maxValue << ".\n";
        cout << colorize(error.str(), UiColor::BOLD_RED);
    }
}

string readLineRequired(const string& prompt) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        line = trim(line);
        if (!line.empty()) return line;
        cout << colorize("This field cannot be empty.\n", UiColor::BOLD_RED);
    }
}

bool parseDateToTm(const string& date, struct tm& out) {
    if (date.size() != 10 || date[2] != '-' || date[5] != '-') return false;
    try {
        int d = stoi(date.substr(0, 2));
        int m = stoi(date.substr(3, 2));
        int y = stoi(date.substr(6, 4));
        if (d < 1 || d > 31 || m < 1 || m > 12 || y < 1900) return false;
        out = {};
        out.tm_mday = d;
        out.tm_mon = m - 1;
        out.tm_year = y - 1900;
        out.tm_isdst = -1;
        time_t normalized = mktime(&out);
        if (normalized == (time_t)-1) return false;
        tm* check = localtime(&normalized);
        return check->tm_mday == d && check->tm_mon == m - 1 && check->tm_year == y - 1900;
    } catch (...) {
        return false;
    }
}

bool isValidDate(const string& date) {
    tm value = {};
    return parseDateToTm(date, value);
}

bool isValidFutureDate(const string& date) {
    tm journeyTm = {};
    if (!parseDateToTm(date, journeyTm)) return false;
    time_t journey = mktime(&journeyTm);
    time_t now = time(nullptr);
    tm* today = localtime(&now);
    today->tm_hour = 0;
    today->tm_min = 0;
    today->tm_sec = 0;
    time_t todayStart = mktime(today);
    return journey >= todayStart;
}

bool parseDateTimeToTime(const string& value, time_t& out) {
    if (value.size() != 16 || value[10] != ' ' || value[13] != ':') return false;
    tm tmValue = {};
    if (!parseDateToTm(value.substr(0, 10), tmValue)) return false;
    try {
        int hour = stoi(value.substr(11, 2));
        int minute = stoi(value.substr(14, 2));
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return false;
        tmValue.tm_hour = hour;
        tmValue.tm_min = minute;
        tmValue.tm_sec = 0;
        tmValue.tm_isdst = -1;
        out = mktime(&tmValue);
        return out != (time_t)-1;
    } catch (...) {
        return false;
    }
}

bool isValidDateTime(const string& value) {
    time_t ignored = 0;
    return parseDateTimeToTime(value, ignored);
}

string statusToString(Status s) {
    if (s == Status::CNF) return "CONFIRMED";
    if (s == Status::RAC) return "RAC";
    if (s == Status::WL) return "WAITLIST";
    return "CANCELLED";
}

Status stringToStatus(const string& s) {
    string v = toUpperStr(trim(s));
    if (v == "CONFIRMED" || v == "CNF") return Status::CNF;
    if (v == "RAC") return Status::RAC;
    if (v == "WAITLIST" || v == "WL") return Status::WL;
    return Status::CANCELLED;
}

string berthPrefToString(BerthPref p) {
    if (p == BerthPref::LOWER) return "Lower";
    if (p == BerthPref::MIDDLE) return "Middle";
    if (p == BerthPref::UPPER) return "Upper";
    return "Any";
}

BerthPref stringToPref(const string& s) {
    string v = toUpperStr(trim(s));
    if (v == "L" || v == "LOWER") return BerthPref::LOWER;
    if (v == "M" || v == "MIDDLE") return BerthPref::MIDDLE;
    if (v == "U" || v == "UPPER") return BerthPref::UPPER;
    return BerthPref::ANY;
}

vector<ClassInfo> classCatalog() {
    return {
        {"SL", "Sleeper", 144, 18, 50, 1.00, true},
        {"1A", "AC Tier 1", 24, 0, 10, 5.00, true},
        {"2A", "AC Tier 2", 48, 10, 25, 3.20, true},
        {"3A", "AC Tier 3", 128, 16, 40, 2.20, true},
        {"CC", "Chair Car", 78, 0, 30, 1.40, false}
    };
}

bool isValidClassCode(const string& code) {
    string normalized = toUpperStr(trim(code));
    for (const ClassInfo& info : classCatalog()) {
        if (info.code == normalized) return true;
    }
    return false;
}

ClassInfo getClassInfo(const string& code) {
    string normalized = toUpperStr(trim(code));
    for (const ClassInfo& info : classCatalog()) {
        if (info.code == normalized) return info;
    }
    return {"SL", "Sleeper", 144, 18, 50, 1.00, true};
}

string classLabel(const string& code) {
    ClassInfo info = getClassInfo(code);
    return info.code + " - " + info.label;
}

double fareForClass(double baseFare, const string& classCode) {
    return baseFare * getClassInfo(classCode).fareMultiplier;
}

string inventoryKey(const string& trainNumber, const string& classCode) {
    return trainNumber + "|" + toUpperStr(trim(classCode));
}

string hashPassword(const string& username, const string& password) {
    const string material = "RAILBOOKER_EDU_SALT_2026|" + username + "|" + password;
    unsigned long long hash = 1469598103934665603ULL;
    for (unsigned char c : material) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    ostringstream out;
    out << hex << hash;
    return out.str();
}

string generatePNR(SystemState& state) {
    while (state.bookings.count(to_string(state.nextPnr))) state.nextPnr++;
    return to_string(state.nextPnr++);
}

static string csvEscape(const string& field) {
    bool quote = field.find_first_of(",\"\n\r") != string::npos;
    string out;
    for (char c : field) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    return quote ? "\"" + out + "\"" : out;
}

static vector<string> parseCsvLine(const string& line) {
    vector<string> fields;
    string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') {
                current += '"';
                i++;
            } else if (c == '"') {
                inQuotes = false;
            } else {
                current += c;
            }
        } else {
            if (c == '"') inQuotes = true;
            else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
    }
    fields.push_back(current);
    return fields;
}

static vector<vector<string>> readCsvRows(const string& path) {
    vector<vector<string>> rows;
    ifstream file(path);
    string line;
    while (getline(file, line)) {
        if (!trim(line).empty()) rows.push_back(parseCsvLine(line));
    }
    return rows;
}

static bool writeCsvRows(const string& path, const vector<vector<string>>& rows) {
    ofstream file(path, ios::trunc);
    if (!file) return false;
    for (const vector<string>& row : rows) {
        for (size_t i = 0; i < row.size(); i++) {
            if (i) file << ',';
            file << csvEscape(row[i]);
        }
        file << '\n';
    }
    return true;
}

static set<int> parseSeatSet(const string& value) {
    set<int> seats;
    string token;
    stringstream ss(value);
    while (getline(ss, token, ';')) {
        token = trim(token);
        if (!token.empty()) {
            try { seats.insert(stoi(token)); } catch (...) {}
        }
    }
    return seats;
}

static string joinSeatSet(const set<int>& seats) {
    ostringstream out;
    bool first = true;
    for (int seat : seats) {
        if (!first) out << ';';
        first = false;
        out << seat;
    }
    return out.str();
}

static deque<string> parseQueue(const string& value) {
    deque<string> q;
    string token;
    stringstream ss(value);
    while (getline(ss, token, ';')) {
        token = trim(token);
        if (!token.empty()) q.push_back(token);
    }
    return q;
}

static string joinQueue(const deque<string>& q) {
    ostringstream out;
    for (size_t i = 0; i < q.size(); i++) {
        if (i) out << ';';
        out << q[i];
    }
    return out.str();
}

static void fillAllSeats(Inventory& inv) {
    inv.availableSeats.clear();
    for (int seat = 1; seat <= inv.totalSeats; seat++) inv.availableSeats.insert(seat);
}

void addLog(SystemState& state, const string& actor, const string& action, const string& details) {
    state.logs.push_back({nowString(), actor, action, details});
}

void rebuildRuntimeIndexes(SystemState& state) {
    state.passengersByPnr.clear();
    state.nextPnr = 1000000001LL;

    for (auto& [key, inv] : state.inventories) {
        inv.racQueue.clear();
        inv.wlQueue.clear();
    }

    map<string, vector<pair<int, string>>> racByInventory;
    map<string, vector<pair<int, string>>> wlByInventory;

    for (auto& [id, p] : state.passengers) {
        state.passengersByPnr[p.pnr].push_back(id);
        try {
            state.nextPnr = max(state.nextPnr, stoll(p.pnr) + 1);
        } catch (...) {
        }
        string key = inventoryKey(p.trainNumber, p.classCode);
        if (p.status == Status::RAC) racByInventory[key].push_back({p.racPosition, id});
        if (p.status == Status::WL) wlByInventory[key].push_back({p.wlPosition, id});
    }

    for (auto& [key, items] : racByInventory) {
        sort(items.begin(), items.end());
        if (!state.inventories.count(key)) continue;
        int pos = 1;
        for (auto& item : items) {
            state.passengers[item.second].racPosition = pos++;
            state.inventories[key].racQueue.push_back(item.second);
        }
    }
    for (auto& [key, items] : wlByInventory) {
        sort(items.begin(), items.end());
        if (!state.inventories.count(key)) continue;
        int pos = 1;
        for (auto& item : items) {
            state.passengers[item.second].wlPosition = pos++;
            state.inventories[key].wlQueue.push_back(item.second);
        }
    }

    for (auto& [pnr, passengerIds] : state.passengersByPnr) {
        sort(passengerIds.begin(), passengerIds.end());
    }
}

bool loadDatabase(SystemState& state) {
    state = SystemState();

    for (auto& row : readCsvRows(USERS_FILE)) {
        if (row.size() >= 4) state.users[row[0]] = {row[0], row[1], row[2], row[3]};
    }
    for (auto& row : readCsvRows(ADMINS_FILE)) {
        if (row.size() >= 2) state.admins[row[0]] = {row[0], row[1]};
    }
    for (auto& row : readCsvRows(TRAINS_FILE)) {
        if (row.size() >= 4) {
            Train t{row[0], row[1], row[2] == "1", 0.0};
            try { t.baseFare = stod(row[3]); } catch (...) { t.baseFare = 0.0; }
            state.trains[t.trainNumber] = t;
        }
    }
    for (auto& row : readCsvRows(STOPS_FILE)) {
        if (row.size() >= 5) {
            Stop s{row[0], 0, toUpperStr(row[2]), row[3], row[4]};
            try { s.sequence = stoi(row[1]); } catch (...) { s.sequence = 0; }
            state.stopsByTrain[s.trainNumber].push_back(s);
        }
    }
    for (auto& [train, stops] : state.stopsByTrain) {
        sort(stops.begin(), stops.end(), [](const Stop& a, const Stop& b) {
            return a.sequence < b.sequence;
        });
    }
    for (auto& row : readCsvRows(INVENTORY_FILE)) {
        if (row.size() >= 8) {
            Inventory inv;
            inv.trainNumber = row[0];
            inv.classCode = toUpperStr(row[1]);
            try { inv.totalSeats = stoi(row[2]); } catch (...) { inv.totalSeats = 0; }
            try { inv.racLimit = stoi(row[3]); } catch (...) { inv.racLimit = 0; }
            try { inv.wlLimit = stoi(row[4]); } catch (...) { inv.wlLimit = 0; }
            inv.availableSeats = parseSeatSet(row[5]);
            inv.racQueue = parseQueue(row[6]);
            inv.wlQueue = parseQueue(row[7]);
            state.inventories[inventoryKey(inv.trainNumber, inv.classCode)] = inv;
        }
    }
    for (auto& row : readCsvRows(BOOKINGS_FILE)) {
        if (row.size() >= 11) {
            Booking b;
            b.pnr = row[0];
            b.username = row[1];
            b.trainNumber = row[2];
            b.trainName = row[3];
            b.source = row[4];
            b.destination = row[5];
            b.journeyDate = row[6];
            b.classCode = row[7];
            b.bookingTime = row[8];
            try { b.totalFare = stod(row[9]); } catch (...) { b.totalFare = 0.0; }
            b.status = stringToStatus(row[10]);
            state.bookings[b.pnr] = b;
        }
    }
    for (auto& row : readCsvRows(PASSENGERS_FILE)) {
        if (row.size() >= 17) {
            Passenger p;
            p.passengerId = row[0];
            p.pnr = row[1];
            p.username = row[2];
            p.name = row[3];
            try { p.age = stoi(row[4]); } catch (...) { p.age = 0; }
            p.preference = stringToPref(row[5]);
            p.classCode = row[6];
            p.trainNumber = row[7];
            p.source = row[8];
            p.destination = row[9];
            p.journeyDate = row[10];
            p.status = stringToStatus(row[11]);
            try { p.seatNumber = stoi(row[12]); } catch (...) { p.seatNumber = -1; }
            try { p.racPosition = stoi(row[13]); } catch (...) { p.racPosition = 0; }
            try { p.wlPosition = stoi(row[14]); } catch (...) { p.wlPosition = 0; }
            try { p.farePaid = stod(row[15]); } catch (...) { p.farePaid = 0.0; }
            state.passengers[p.passengerId] = p;
        }
    }
    for (auto& row : readCsvRows(LOGS_FILE)) {
        if (row.size() >= 4) state.logs.push_back({row[0], row[1], row[2], row[3]});
    }
    for (auto& row : readCsvRows(TRANSACTIONS_FILE)) {
        if (row.size() >= 6) {
            Transaction tx{row[0], row[1], row[2], row[3], 0.0, row[5]};
            try { tx.amount = stod(row[4]); } catch (...) { tx.amount = 0.0; }
            state.transactions.push_back(tx);
        }
    }

    rebuildRuntimeIndexes(state);
    return true;
}

bool saveDatabase(const SystemState& state) {
    vector<vector<string>> rows;

    rows.clear();
    for (const auto& [username, user] : state.users) {
        rows.push_back({user.username, user.passwordHash, user.fullName, user.contact});
    }
    if (!writeCsvRows(USERS_FILE, rows)) return false;

    rows.clear();
    for (const auto& [username, admin] : state.admins) {
        rows.push_back({admin.username, admin.passwordHash});
    }
    if (!writeCsvRows(ADMINS_FILE, rows)) return false;

    rows.clear();
    for (const auto& [trainNo, train] : state.trains) {
        rows.push_back({train.trainNumber, train.trainName, train.active ? "1" : "0", to_string(train.baseFare)});
    }
    if (!writeCsvRows(TRAINS_FILE, rows)) return false;

    rows.clear();
    for (const auto& [trainNo, stops] : state.stopsByTrain) {
        for (const Stop& stop : stops) {
            rows.push_back({stop.trainNumber, to_string(stop.sequence), stop.station, stop.arrival, stop.departure});
        }
    }
    if (!writeCsvRows(STOPS_FILE, rows)) return false;

    rows.clear();
    for (const auto& [key, inv] : state.inventories) {
        rows.push_back({
            inv.trainNumber,
            inv.classCode,
            to_string(inv.totalSeats),
            to_string(inv.racLimit),
            to_string(inv.wlLimit),
            joinSeatSet(inv.availableSeats),
            joinQueue(inv.racQueue),
            joinQueue(inv.wlQueue)
        });
    }
    if (!writeCsvRows(INVENTORY_FILE, rows)) return false;

    rows.clear();
    for (const auto& [pnr, b] : state.bookings) {
        rows.push_back({
            b.pnr, b.username, b.trainNumber, b.trainName, b.source, b.destination,
            b.journeyDate, b.classCode, b.bookingTime, to_string(b.totalFare), statusToString(b.status)
        });
    }
    if (!writeCsvRows(BOOKINGS_FILE, rows)) return false;

    rows.clear();
    for (const auto& [id, p] : state.passengers) {
        rows.push_back({
            p.passengerId,
            p.pnr,
            p.username,
            p.name,
            to_string(p.age),
            berthPrefToString(p.preference),
            p.classCode,
            p.trainNumber,
            p.source,
            p.destination,
            p.journeyDate,
            statusToString(p.status),
            to_string(p.seatNumber),
            to_string(p.racPosition),
            to_string(p.wlPosition),
            to_string(p.farePaid),
            "v1"
        });
    }
    if (!writeCsvRows(PASSENGERS_FILE, rows)) return false;

    rows.clear();
    for (const LogEntry& log : state.logs) {
        rows.push_back({log.timestamp, log.actor, log.action, log.details});
    }
    if (!writeCsvRows(LOGS_FILE, rows)) return false;

    rows.clear();
    for (const Transaction& tx : state.transactions) {
        rows.push_back({tx.timestamp, tx.pnr, tx.username, tx.type, to_string(tx.amount), tx.details});
    }
    return writeCsvRows(TRANSACTIONS_FILE, rows);
}

static void addInventoryForClass(SystemState& state, const string& trainNumber, const string& classCode) {
    ClassInfo info = getClassInfo(classCode);
    Inventory inv;
    inv.trainNumber = trainNumber;
    inv.classCode = info.code;
    inv.totalSeats = info.totalSeats;
    inv.racLimit = info.racLimit;
    inv.wlLimit = info.wlLimit;
    fillAllSeats(inv);
    state.inventories[inventoryKey(trainNumber, info.code)] = inv;
}

static void addDemoTrain(SystemState& state, const string& trainNumber, const string& name,
                         double baseFare, const vector<string>& classes,
                         const vector<pair<string, pair<string, string>>>& stops) {
    state.trains[trainNumber] = {trainNumber, name, true, baseFare};
    vector<Stop> registeredStops;
    for (size_t i = 0; i < stops.size(); i++) {
        registeredStops.push_back({
            trainNumber,
            (int)i + 1,
            toUpperStr(stops[i].first),
            stops[i].second.first,
            stops[i].second.second
        });
    }
    state.stopsByTrain[trainNumber] = registeredStops;
    for (const string& cls : classes) addInventoryForClass(state, trainNumber, cls);
}

static void seedDefaultsIfNeeded(SystemState& state) {
    bool changed = false;
    if (state.admins.empty()) {
        state.admins["admin"] = {"admin", hashPassword("admin", "admin123")};
        addLog(state, "SYSTEM", "ADMIN_SEEDED", "Default admin admin/admin123 created");
        changed = true;
    }

    if (state.trains.empty()) {
        string d1 = dateOffsetString(1);
        string d2 = dateOffsetString(2);
        addDemoTrain(state, "12951", "Mumbai Rajdhani", 1200.0, {"1A", "2A", "3A"}, {
            {"NEW DELHI", {d1 + " 16:55", d1 + " 17:00"}},
            {"KOTA", {d1 + " 21:30", d1 + " 21:35"}},
            {"VADODARA", {d2 + " 04:25", d2 + " 04:35"}},
            {"MUMBAI CENTRAL", {d2 + " 08:35", d2 + " 08:45"}}
        });
        addDemoTrain(state, "12627", "Karnataka Express", 900.0, {"SL", "2A", "3A"}, {
            {"NEW DELHI", {d1 + " 20:10", d1 + " 20:20"}},
            {"BHOPAL", {d2 + " 05:40", d2 + " 05:50"}},
            {"BENGALURU", {d2 + " 23:30", d2 + " 23:40"}}
        });
        addDemoTrain(state, "12002", "Shatabdi Chair Car", 600.0, {"CC"}, {
            {"NEW DELHI", {d1 + " 06:00", d1 + " 06:05"}},
            {"AGRA", {d1 + " 08:00", d1 + " 08:05"}},
            {"BHOPAL", {d1 + " 14:00", d1 + " 14:10"}}
        });
        addLog(state, "SYSTEM", "DEMO_TRAINS_SEEDED", "Initial demo train database created");
        changed = true;
    }

    if (changed) saveDatabase(state);
}

bool trainServesRoute(const SystemState& state, const string& trainNumber,
                      const string& source, const string& destination,
                      const string& date, int* sourceIndex, int* destinationIndex) {
    auto it = state.stopsByTrain.find(trainNumber);
    if (it == state.stopsByTrain.end()) return false;

    string src = toUpperStr(trim(source));
    string dst = toUpperStr(trim(destination));
    int srcIdx = -1;
    int dstIdx = -1;
    for (size_t i = 0; i < it->second.size(); i++) {
        if (it->second[i].station == src && srcIdx == -1) srcIdx = (int)i;
        if (it->second[i].station == dst && srcIdx != -1) {
            dstIdx = (int)i;
            break;
        }
    }
    if (srcIdx == -1 || dstIdx == -1 || srcIdx >= dstIdx) return false;
    if (!date.empty() && it->second[srcIdx].departure.substr(0, 10) != date) return false;
    if (sourceIndex) *sourceIndex = srcIdx;
    if (destinationIndex) *destinationIndex = dstIdx;
    return true;
}

vector<string> classesForTrain(const SystemState& state, const string& trainNumber) {
    vector<string> classes;
    for (const ClassInfo& info : classCatalog()) {
        if (state.inventories.count(inventoryKey(trainNumber, info.code))) classes.push_back(info.code);
    }
    return classes;
}

static vector<string> parseClassSelection(const string& input) {
    string normalized = toUpperStr(trim(input));
    vector<string> selected;
    if (normalized == "ALL") {
        for (const ClassInfo& info : classCatalog()) selected.push_back(info.code);
        return selected;
    }

    string token;
    for (char c : normalized) {
        if (c == ',' || isspace((unsigned char)c)) {
            if (!token.empty()) {
                if (isValidClassCode(token) && find(selected.begin(), selected.end(), token) == selected.end())
                    selected.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty() && isValidClassCode(token) && find(selected.begin(), selected.end(), token) == selected.end()) {
        selected.push_back(token);
    }
    return selected;
}

void adminRegisterTrainMenu(SystemState& state, const string& adminName) {
    clearScreen();
    cout << "=== ADMIN: REGISTER TRAIN ===\n\n";

    string trainNumber = readLineRequired("Train number: ");
    if (state.trains.count(trainNumber)) {
        cout << "Train already exists. Registration cancelled.\n";
        pauseScreen();
        return;
    }

    string trainName = readLineRequired("Train name: ");
    double baseFare = readDouble("Base fare for Sleeper equivalent: Rs. ", 1.0, 100000.0);
    int stopCount = readInt("Number of stops (minimum 2): ", 2, 50);

    vector<Stop> stops;
    for (int i = 1; i <= stopCount; i++) {
        cout << "\nStop " << i << "\n";
        string station = toUpperStr(readLineRequired("Station name: "));
        string arrival;
        string departure;
        while (true) {
            arrival = readLineRequired("Arrival (DD-MM-YYYY HH:MM): ");
            if (isValidDateTime(arrival)) break;
            cout << "Invalid date/time format.\n";
        }
        while (true) {
            departure = readLineRequired("Departure (DD-MM-YYYY HH:MM): ");
            if (isValidDateTime(departure)) break;
            cout << "Invalid date/time format.\n";
        }
        stops.push_back({trainNumber, i, station, arrival, departure});
    }

    cout << "\nAvailable class codes: SL, 1A, 2A, 3A, CC\n";
    cout << "Enter comma-separated classes or ALL.\n";
    vector<string> selectedClasses;
    while (selectedClasses.empty()) {
        selectedClasses = parseClassSelection(readLineRequired("Classes: "));
        if (selectedClasses.empty()) cout << "Choose at least one valid class.\n";
    }

    state.trains[trainNumber] = {trainNumber, trainName, true, baseFare};
    state.stopsByTrain[trainNumber] = stops;
    for (const string& cls : selectedClasses) addInventoryForClass(state, trainNumber, cls);

    addLog(state, adminName, "TRAIN_REGISTERED", trainNumber + " - " + trainName);
    saveDatabase(state);

    cout << "\nTrain registered successfully with " << selectedClasses.size() << " class type(s).\n";
    pauseScreen();
}

static void changeAdminPassword(SystemState& state, const string& adminName) {
    clearScreen();
    cout << "=== CHANGE ADMIN PASSWORD ===\n\n";
    string oldPassword = readLineRequired("Current password: ");
    if (state.admins[adminName].passwordHash != hashPassword(adminName, oldPassword)) {
        cout << "Incorrect password.\n";
        pauseScreen();
        return;
    }
    string newPassword = readLineRequired("New password: ");
    state.admins[adminName].passwordHash = hashPassword(adminName, newPassword);
    addLog(state, adminName, "ADMIN_PASSWORD_CHANGED", "Admin changed own password");
    saveDatabase(state);
    cout << "Password changed.\n";
    pauseScreen();
}

static void adminMenu(SystemState& state, const string& adminName) {
    while (true) {
        clearScreen();
        cout << colorize("=== ADMIN MENU (" + adminName + ") ===\n", UiColor::BOLD_CYAN);
        cout << "1. Register Train\n";
        cout << "2. View Trains\n";
        cout << "3. View Bookings\n";
        cout << "4. View Logs\n";
        cout << "5. Change Admin Password\n";
        cout << "6. Logout\n";
        int choice = readInt("Choice: ", 1, 6);
        if (choice == 1) adminRegisterTrainMenu(state, adminName);
        else if (choice == 2) adminViewTrains(state);
        else if (choice == 3) adminViewBookings(state);
        else if (choice == 4) adminViewLogs(state);
        else if (choice == 5) changeAdminPassword(state, adminName);
        else break;
    }
}

static void userMenu(SystemState& state, const string& username) {
    while (true) {
        clearScreen();
        cout << colorize("=== USER MENU (" + username + ") ===\n", UiColor::BOLD_CYAN);
        cout << "1. Book Ticket\n";
        cout << "2. Cancel Ticket\n";
        cout << "3. View My PNR Status\n";
        cout << "4. View My Booking History\n";
        cout << "5. View Trains & Dates\n";
        cout << "6. Train Availability\n";
        cout << "7. Waitlist Prediction\n";
        cout << "8. Logout\n";
        int choice = readInt("Choice: ", 1, 8);
        if (choice == 1) bookingMenu(state, username);
        else if (choice == 2) cancellationMenu(state, username);
        else if (choice == 3) pnrStatusMenu(state, username);
        else if (choice == 4) bookingHistoryMenu(state, username);
        else if (choice == 5) userViewTrains(state);
        else if (choice == 6) trainAvailabilityMenu(state);
        else if (choice == 7) predictionMenu(state, username);
        else break;
    }
}

static void registerUser(SystemState& state) {
    clearScreen();
    cout << colorize("=== USER REGISTRATION ===\n\n", UiColor::BOLD_CYAN);
    string username = readLineRequired("Username: ");
    if (state.users.count(username)) {
        cout << colorize("Username already exists.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }
    string password = readLineRequired("Password: ");
    string fullName = readLineRequired("Full name: ");
    string contact = readLineRequired("Phone/email: ");
    state.users[username] = {username, hashPassword(username, password), fullName, contact};
    addLog(state, username, "USER_REGISTERED", "User account created");
    saveDatabase(state);
    cout << colorize("\nRegistration complete. You can log in now.\n", UiColor::BOLD_GREEN);
    pauseScreen();
}

static void loginUser(SystemState& state) {
    clearScreen();
    cout << colorize("=== USER LOGIN ===\n\n", UiColor::BOLD_CYAN);
    string username = readLineRequired("Username: ");
    string password = readLineRequired("Password: ");
    auto it = state.users.find(username);
    if (it == state.users.end() || it->second.passwordHash != hashPassword(username, password)) {
        addLog(state, username, "USER_LOGIN_FAILED", "Invalid credentials");
        saveDatabase(state);
        cout << colorize("Invalid username or password.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }
    addLog(state, username, "USER_LOGIN", "User logged in");
    saveDatabase(state);
    userMenu(state, username);
}

static void loginAdmin(SystemState& state) {
    clearScreen();
    cout << colorize("=== ADMIN LOGIN ===\n\n", UiColor::BOLD_CYAN);
    string username = readLineRequired("Admin username: ");
    string password = readLineRequired("Admin password: ");
    auto it = state.admins.find(username);
    if (it == state.admins.end() || it->second.passwordHash != hashPassword(username, password)) {
        addLog(state, username, "ADMIN_LOGIN_FAILED", "Invalid credentials");
        saveDatabase(state);
        cout << colorize("Invalid admin credentials.\n", UiColor::BOLD_RED);
        pauseScreen();
        return;
    }
    addLog(state, username, "ADMIN_LOGIN", "Admin logged in");
    saveDatabase(state);
    adminMenu(state, username);
}

int main() {
    enableAnsiColors();
    SystemState state;
    loadDatabase(state);
    seedDefaultsIfNeeded(state);

    while (true) {
        clearScreen();
        cout << colorize("=====================================\n", UiColor::BOLD_CYAN);
        cout << colorize(" IRCTC-STYLE RAILBOOKER - C++ APP\n", UiColor::BOLD_WHITE);
        cout << colorize("=====================================\n", UiColor::BOLD_CYAN);
        cout << colorize("1. Check PNR Status\n", UiColor::BOLD_GREEN);
        cout << colorize("2. Search Train by Number / Route\n", UiColor::BOLD_GREEN);
        cout << "3. User Login\n";
        cout << "4. User Registration\n";
        cout << "5. Admin Login\n";
        cout << "6. Exit\n";
        int choice = readInt("Choice: ", 1, 6);
        if (choice == 1) publicPnrStatusMenu(state);
        else if (choice == 2) trainNumberSearchMenu(state);
        else if (choice == 3) loginUser(state);
        else if (choice == 4) registerUser(state);
        else if (choice == 5) loginAdmin(state);
        else {
            saveDatabase(state);
            clearScreen();
            cout << colorize("Thank you for using Railbooker.\n", UiColor::BOLD_GREEN);
            break;
        }
    }
    return 0;
}
