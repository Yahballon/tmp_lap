// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
namespace pstube {
struct Video {std::string id,title,author,description,thumbnail;int length=0;long views=0;bool live=false;};
struct Job {std::string id,title;float progress=0;std::string state,error,path;};
struct Snapshot {std::vector<Video> videos;std::vector<Job> jobs;std::string status="Search for videos to begin";bool searching=false,loadingMore=false,hasMore=false;std::string streamId,streamUrl,streamError;bool streamResolving=false;};
class Service {
 public:
 Service();~Service();
 bool search(const std::string& query);
 bool trending();
 bool load_more();
 bool start_stream(const Video& video);
 void cancel_stream();
 bool details(const std::string& id);
 bool download(const Video& video);
 std::string local_path(const std::string& videoId) const;
 Snapshot snapshot() const;
 std::vector<std::string> favourites()const;
 std::vector<Video> favorite_videos()const;
 std::vector<Video> history_videos()const;
 void record_history(const Video& video);
 bool toggle_favourite(const Video& video);
 bool toggle_favourite(const std::string& id);
 bool is_favourite(const std::string& id)const;
 void set_instance(const std::string& hostname);
 std::string instance()const;
 private:
 mutable std::mutex mutex_;
 std::atomic<bool> busy_{false},downloading_{false},resolvingStream_{false};
 std::string currentQuery_,continuation_,clientVersion_,pagingProvider_;
 int providerPage_=1;
 uint64_t feedGeneration_=0;
 std::atomic<int> download_progress_{0};
 std::string downloading_id_;
 std::string instance_="yewtu.be";
 std::vector<std::string> fav_;
 std::vector<Video> favorite_meta_,recent_meta_;
 Snapshot state_;
 bool persist() const;
};
struct ThumbnailHealth {int successful=0,failed=0,pending=0;std::string lastError;std::string directory;};
void queue_thumbnails(const std::vector<Video>& videos);
std::string thumbnail_file(const std::string& videoId);
ThumbnailHealth thumbnail_health();
bool valid_id(const std::string&);
std::vector<Video> parse_search(const std::string&);
std::vector<Video> parse_youtube_html(const std::string&);
std::string json_string(const std::string&,const std::string&);
std::string escape_url(const std::string&);
// Native HTTPS Innertube resolver. Returned URLs are ephemeral; never persist them.
bool resolve_android_mp4(const std::string& videoId,std::string& mediaUrl,std::string& error);
}