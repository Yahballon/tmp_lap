// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
namespace motion {
struct Entry {
 std::string slug,title,category,url,preview,tags;
};
class Catalog {
public:
 bool load(const std::string& assets);
 const std::vector<Entry>& entries() const {return records_;}
 bool download(int index);
 bool request_thumbnail(int index);
 std::vector<int> consume_ready();
 std::string thumbnail_path(int index) const;
 std::string wallpaper_path(int index) const;
 bool installed(int index) const;
 int progress() const {return percent_.load();}
 bool downloading() const {return downloading_.load();}
 bool thumbnail_busy() const {return preview_busy_.load();}
 std::string status() const;
private:
 struct Worker {Catalog* self;int index;bool preview;};
 static void* run(void*);
 bool start(int index,bool preview);
 void report(std::string message);
 std::vector<Entry> records_;
 mutable std::mutex mutex_;
 std::vector<int> ready_;
 std::vector<bool> preview_requested_;
 std::vector<bool> installed_cache_;
 std::atomic<bool> downloading_{false};
 std::atomic<bool> preview_busy_{false};
 std::atomic<int> percent_{0};
 std::string status_="Choose wallpaper and press X to download / install";
};
}
