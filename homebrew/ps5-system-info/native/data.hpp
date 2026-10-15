// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace info {
struct Row {std::string label, value; bool available=true;};
struct App {std::string title,id; std::uint64_t bytes=0;};
struct Snapshot {
 std::string time, uptime, localIp, route, clockSource;
 std::uint64_t uptimeSeconds=0;
 double ownCpuSeconds=0;
 long ownPeakRss=0;
 int ownPid=0;
 int homebrewFolders=0;
 int homebrewExecutables=0;
 int storageNodes=0;
 bool controller=false;
 std::vector<App> apps;
 std::vector<Row> system,hardware,storage,network,video,audio,power,peripherals,processes,kernel,logs;
};
struct Settings {int refreshSeconds=10;bool showUnavailable=true;int theme=0;};
Snapshot collect(int displayWidth,int displayHeight,bool padConnected);
Settings loadSettings();
bool saveSettings(const Settings&);
bool exportReport(const Snapshot&,const Settings&,std::string& outPath);
bool appendLog(const std::string&);
std::vector<std::string> recentLogs();
std::string compactBytes(std::uint64_t);
std::string unavailable();
}