// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
namespace torrent {
struct Snapshot {
    std::string name, status, tracker, infoHash, cacheDir, downloadDir, lastError;
    uint64_t totalBytes=0, completedBytes=0, transferredBytes=0, uploadedBytes=0;
    double downBps=0, upBps=0;
    int pieceCount=0, verifiedPieces=0, peerCount=0, activePeers=0;
    bool running=false, paused=false, done=false, loaded=false, usb=false;
};
class Engine {
public:
    Engine();
    Snapshot snapshot() const;
    bool start(const std::string& path);
    void stop();
    void setUsb(bool enabled);
    bool usingUsb() const;
private:
    void run(std::string path);
    void publish(const std::string& message);
    mutable std::mutex mutex_;
    Snapshot state_;
    bool busy_=false;
    bool stopRequested_=false;
};
Engine& engine();
const char* sampleTorrentName();
const char* sampleTorrentPath();
std::vector<std::string> discoverTorrents();
bool createDirectories(const std::string& path);
}
