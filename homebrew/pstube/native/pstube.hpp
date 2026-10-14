// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
namespace pstube {
struct Video {std::string id,title,author,description,thumbnail;int length=0;long views=0;bool live=false;};
struct Job {std::string id,title;float progress=0;std::string state,error,path;};
struct Snapshot {std::vector<Video> videos;std::vector<Job> jobs;std::string status="Search for videos to begin";bool searching=false;};
class Service {
 public:
 Service();~Service();
 bool search(const std::string& query);
 bool trending();
 bool details(const std::string& id);
 bool download(const Video& video);
 std::string local_path(const std::string& videoId) const;
 Snapshot snapshot() const;
 std::vector<std::string> favourites()const;
 bool toggle_favourite(const std::string& id);
 bool is_favourite(const std::string& id)const;
 void set_instance(const std::string& hostname);
 std::string instance()const;
 private:
 mutable std::mutex mutex_;
 std::atomic<bool> busy_{false},downloading_{false};
 std::atomic<int> download_progress_{0};
 std::string downloading_id_;
 std::string instance_="yewtu.be";
 std::vector<std::string> fav_;
 Snapshot state_;
 bool persist() const;
};
void queue_thumbnails(const std::vector<Video>& videos);
bool valid_id(const std::string&);
std::vector<Video> parse_search(const std::string&);
std::string json_string(const std::string&,const std::string&);
std::string escape_url(const std::string&);
}