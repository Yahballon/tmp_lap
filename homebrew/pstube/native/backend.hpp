// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <deque>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>

namespace pstube {
struct Video {
    std::string id, title, author;
    int duration=0;
    std::uint64_t views=0;
};
struct Download {
    Video video;
    std::string state="Queued";
    int progress=0;
    bool autoPlay=false;
};
struct Snapshot {
    std::vector<Video> videos, favorites, history;
    std::vector<Download> downloads;
    std::string status="Starting...", query, instance;
    bool loading=false;
    bool isSearch=false;
    int page=1;
};
class Backend {
public:
    Backend();
    ~Backend();
    Backend(const Backend&)=delete;
    Backend& operator=(const Backend&)=delete;
    Snapshot snapshot();
    void search(const std::string& query,int page=1);
    void trending();
    void toggleFavorite(const Video& v);
    bool favorite(const std::string& id);
    void remember(const Video& v);
    void enqueue(const Video& v,bool play);
    void setInstance(int index);
    int instanceIndex();
    std::string cachePath(const std::string& id) const;
    std::string downloadPath(const std::string& id) const;
    std::string readyToPlay();
    std::string root() const { return root_; }
private:
    struct Task { int type=0; Video video; std::string query; int page=1; bool play=false; };
    std::mutex mutex_;
    pthread_t worker_{};
    bool running_=false, quit_=false;
    std::deque<Task> queue_;
    Snapshot state_;
    int instance_=0;
    std::string root_, ready_;
    static void* run(void* p);
    void loop();
    void saveList(const std::vector<Video>& items,const std::string& name);
    std::vector<Video> loadList(const std::string& name);
};
bool validId(const std::string& s);
std::string shortText(std::string s,std::size_t max);
std::string durationText(int seconds);
}