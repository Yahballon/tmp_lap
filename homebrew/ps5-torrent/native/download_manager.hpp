// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "torrent_engine.hpp"
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>
namespace torrent {
struct QueueSettings {
 bool autoDownload=false, deleteCompletedTorrents=true, usb=false;
 int maxActive=2, scanSeconds=3;
};
struct JobInfo {
 std::string name, source, hash, status, phase;
 uint64_t total=0, completed=0;
 double speed=0;
 int peers=0, activePeers=0;
 bool queued=false, running=false, completedJob=false, failed=false, cancelled=false;
};
class DownloadManager {
public:
 DownloadManager();
 ~DownloadManager();
 DownloadManager(const DownloadManager&)=delete;
 DownloadManager& operator=(const DownloadManager&)=delete;
 bool enqueue(const std::string& path,std::string* error=nullptr,bool retryExisting=true);
 int enqueueAll(const std::vector<std::string>& paths);
 void tick();
 void scan(bool force=false);
 void cancel(size_t index);
 void retry(size_t index);
 int retryAllFailed();
 void stopAll();
 bool isRecoveryMode() const {return recoveryMode_;}
 bool resumeAfterRecovery();
 std::vector<JobInfo> jobs() const;
 QueueSettings settings() const { return settings_; }
 void setAutoDownload(bool enabled);
 void setMaxActive(int number);
 void setDeleteCompleted(bool enabled);
 bool setUsbStorage(bool enabled);
 void setScanSeconds(int seconds);
 std::string torrentDirectory() const;
 std::string iniPath() const;
 std::string queuePath() const;
private:
 struct Job{
  std::string path, hash, name, state="queued", message="Queued";
  uint64_t total=0;
  std::unique_ptr<Engine> engine;
 };
 std::vector<std::unique_ptr<Job>> jobs_;
 QueueSettings settings_;
 std::map<std::string,std::string> observed_;
 std::chrono::steady_clock::time_point lastScan_;
 bool inTick_=false;
 bool recoveryMode_=false;
 std::string sessionMarker() const;
 std::string recoveryMarker() const;
 std::string metadataCachePath(const std::string& hash) const;
 bool resolveMetadata(Job& job,std::string& chosen);
 void loadSettings();
 void saveSettings() const;
 void loadQueue();
 void saveQueue() const;
 bool safeDeleteSource(const std::string& path) const;
 void launchPending();
};
}
