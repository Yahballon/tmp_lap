// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tracker {

struct Coin {
    std::string id;
    std::string name;
    std::string symbol;
    int rank = 0;
    double price = 0;
    double change24h = 0;
    double marketCap = 0;
    double volume24h = 0;
    double circulating = 0;
    double totalSupply = 0;
    double ath = 0;
    double atl = 0;
    std::vector<double> spark;
};

struct Snapshot {
    std::vector<Coin> coins;
    std::vector<double> chart;
    std::string chartCoin;
    int chartDays = 0;
    bool marketLoaded = false;
    bool chartLoaded = false;
    bool busy = false;
    std::string status = "Press TRIANGLE to fetch live prices";
    std::uint64_t refreshedAtUs = 0;
};

// One bounded HTTPS request at a time, off the rendering thread.
// Never replaces valid cached quotes with fabricated or zero prices on errors.
bool requestMarket();
bool requestChart(const std::string& coinId, int days);
Snapshot snapshot();
} // namespace tracker
