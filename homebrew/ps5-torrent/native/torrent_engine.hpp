// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <atomic>
#include <mutex>
#include <memory>
#include <thread>
#include <string>
#include <vector>
namespace torrent {
struct SocketRegistry;
struct TorrentInfo { std::string name, infoHash, tracker; uint64_t bytes=0; };
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
    ~Engine();
    Engine(const Engine&)=delete;
    Engine& operator=(const Engine&)=delete;
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
    std::atomic<bool> cancellation_{false};
    std::shared_ptr<SocketRegistry> sockets_;
    std::thread runner_;
};
Engine& engine();
bool inspectTorrent(const std::string& path,TorrentInfo& output,std::string& error);
std::string storageRoot();
const char* sampleTorrentName();
const char* sampleTorrentPath();
std::vector<std::string> discoverTorrents();
std::string testDisplayName(const std::string& path);
std::string testSizeHint(const std::string& path);
bool createDirectories(const std::string& path);
std::string logPath();
std::vector<std::string> recentLogs();
}
