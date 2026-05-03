#pragma once

#include <algorithm>
#include <cctype>
#include <ctime>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

enum class Status { CNF, RAC, WL, CANCELLED };
enum class BerthPref { LOWER, MIDDLE, UPPER, ANY };
enum class UiColor { RESET, DIM, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, BOLD_RED, BOLD_GREEN, BOLD_YELLOW, BOLD_CYAN, BOLD_WHITE };

struct ClassInfo {
    string code;
    string label;
    int totalSeats;
    int racLimit;
    int wlLimit;
    double fareMultiplier;
    bool berthBased;
};

struct UserAccount {
    string username;
    string passwordHash;
    string fullName;
    string contact;
};

struct AdminAccount {
    string username;
    string passwordHash;
};

struct Train {
    string trainNumber;
    string trainName;
    bool active;
    double baseFare;
};

struct Stop {
    string trainNumber;
    int sequence;
    string station;
    string arrival;
    string departure;
};

struct Inventory {
    string trainNumber;
    string classCode;
    int totalSeats;
    int racLimit;
    int wlLimit;
    set<int> availableSeats;
    deque<string> racQueue;
    deque<string> wlQueue;
};

struct Booking {
    string pnr;
    string username;
    string trainNumber;
    string trainName;
    string source;
    string destination;
    string journeyDate;
    string classCode;
    string bookingTime;
    double totalFare;
    Status status;
};

struct Passenger {
    string passengerId;
    string pnr;
    string username;
    string name;
    int age;
    BerthPref preference;
    string classCode;
    string trainNumber;
    string source;
    string destination;
    string journeyDate;
    Status status;
    int seatNumber;
    int racPosition;
    int wlPosition;
    double farePaid;
};

struct LogEntry {
    string timestamp;
    string actor;
    string action;
    string details;
};

struct Transaction {
    string timestamp;
    string pnr;
    string username;
    string type;
    double amount;
    string details;
};

struct SystemState {
    unordered_map<string, UserAccount> users;
    unordered_map<string, AdminAccount> admins;
    unordered_map<string, Train> trains;
    map<string, vector<Stop>> stopsByTrain;
    map<string, Inventory> inventories;
    unordered_map<string, Booking> bookings;
    unordered_map<string, Passenger> passengers;
    map<string, vector<string>> passengersByPnr;
    vector<LogEntry> logs;
    vector<Transaction> transactions;
    long long nextPnr = 1000000001LL;
};

string trim(const string& s);
string toUpperStr(const string& s);
string nowString();
string dateOffsetString(int daysFromToday);
void enableAnsiColors();
string colorize(const string& text, UiColor color);
string statusColored(Status s);
string statusColoredPadded(Status s, int width);
void clearScreen();
void pauseScreen();
int readInt(const string& prompt, int minValue, int maxValue);
double readDouble(const string& prompt, double minValue, double maxValue);
string readLineRequired(const string& prompt);

bool parseDateToTm(const string& date, struct tm& out);
bool parseDateTimeToTime(const string& value, time_t& out);
bool isValidDate(const string& date);
bool isValidFutureDate(const string& date);
bool isValidDateTime(const string& value);

string statusToString(Status s);
Status stringToStatus(const string& s);
string berthPrefToString(BerthPref p);
BerthPref stringToPref(const string& s);

vector<ClassInfo> classCatalog();
bool isValidClassCode(const string& code);
ClassInfo getClassInfo(const string& code);
string classLabel(const string& code);
double fareForClass(double baseFare, const string& classCode);
string inventoryKey(const string& trainNumber, const string& classCode);
string hashPassword(const string& username, const string& password);
string generatePNR(SystemState& state);

bool loadDatabase(SystemState& state);
bool saveDatabase(const SystemState& state);
void rebuildRuntimeIndexes(SystemState& state);
void addLog(SystemState& state, const string& actor, const string& action, const string& details);

bool trainServesRoute(const SystemState& state, const string& trainNumber,
                      const string& source, const string& destination,
                      const string& date, int* sourceIndex = nullptr,
                      int* destinationIndex = nullptr);
vector<string> classesForTrain(const SystemState& state, const string& trainNumber);

void bookingMenu(SystemState& state, const string& username);
void cancellationMenu(SystemState& state, const string& username);
void publicPnrStatusMenu(SystemState& state);
void trainNumberSearchMenu(SystemState& state);
void pnrStatusMenu(SystemState& state, const string& username);
void bookingHistoryMenu(SystemState& state, const string& username);
void userViewTrains(SystemState& state);
void trainAvailabilityMenu(SystemState& state);
void predictionMenu(SystemState& state, const string& username);
void displayWaitlist(SystemState& state);

void adminRegisterTrainMenu(SystemState& state, const string& adminName);
void adminViewTrains(SystemState& state);
void adminViewBookings(SystemState& state);
void adminViewLogs(SystemState& state);

void promoteRACToCNF(SystemState& state, const string& trainNumber, const string& classCode);
void promoteWaitlistToRAC(SystemState& state, const string& trainNumber, const string& classCode);
void refreshBookingStatus(SystemState& state, const string& pnr);
