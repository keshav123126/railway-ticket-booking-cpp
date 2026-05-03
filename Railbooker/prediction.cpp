#include "common.h"

static vector<pair<int, double>> historicalCurve() {
    return {
        {1, 95.0}, {2, 90.0}, {3, 85.0}, {5, 78.0},
        {8, 68.0}, {10, 60.0}, {15, 45.0}, {20, 30.0},
        {25, 18.0}, {30, 10.0}, {40, 5.0}, {50, 2.0}
    };
}

static double interpolateProbability(int wlPos) {
    vector<pair<int, double>> data = historicalCurve();
    if (wlPos <= data.front().first) return data.front().second;
    if (wlPos >= data.back().first) return data.back().second;

    int lo = 0;
    int hi = (int)data.size() - 1;
    while (lo + 1 < hi) {
        int mid = (lo + hi) / 2;
        if (data[mid].first <= wlPos) lo = mid;
        else hi = mid;
    }
    double ratio = (double)(wlPos - data[lo].first) / (data[hi].first - data[lo].first);
    return data[lo].second + ratio * (data[hi].second - data[lo].second);
}

static string estimateText(int wlPos) {
    if (wlPos <= 5) return "Likely within 1 day";
    if (wlPos <= 15) return "Likely within 3 days";
    if (wlPos <= 30) return "Possible within 7 days";
    return "Unlikely; consider alternatives";
}

void predictionMenu(SystemState& state, const string& username) {
    clearScreen();
    cout << "=== WAITLIST PREDICTION ===\n\n";
    string pnr = readLineRequired("Enter PNR: ");
    if (!state.bookings.count(pnr)) {
        cout << "PNR not found.\n";
        pauseScreen();
        return;
    }
    if (state.bookings[pnr].username != username) {
        cout << "This PNR belongs to another user.\n";
        pauseScreen();
        return;
    }

    bool anyWaitlisted = false;
    auto idsIt = state.passengersByPnr.find(pnr);
    if (idsIt != state.passengersByPnr.end()) {
        cout << left << setw(24) << "Passenger" << setw(8) << "WL#"
             << setw(12) << "Confirm %" << "Estimate\n";
        cout << string(68, '-') << "\n";
        for (const string& id : idsIt->second) {
            if (!state.passengers.count(id)) continue;
            const Passenger& p = state.passengers[id];
            if (p.status != Status::WL) continue;
            anyWaitlisted = true;
            double probability = interpolateProbability(p.wlPosition);
            cout << left << setw(24) << p.name.substr(0, 23)
                 << setw(8) << p.wlPosition
                 << fixed << setprecision(1) << setw(12) << probability
                 << estimateText(p.wlPosition) << "\n";
        }
    }

    if (!anyWaitlisted) {
        cout << "Prediction is only needed for waitlisted passengers. This PNR has no active WL passenger.\n";
    }
    pauseScreen();
}
