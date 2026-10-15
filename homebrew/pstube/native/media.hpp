// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>
namespace pstube {
struct PlaybackState {
 bool opening=false,playing=false,paused=false,ended=false;
 int width=0,height=0; double seconds=0;
 std::string message;
};
class MediaPlayer {
public:
 MediaPlayer()=default;~MediaPlayer();
 bool open(const std::string& path);
 bool openStream(const std::string& url);
 void close();
 void pause();
 PlaybackState snapshot() const;
 bool pull(std::vector<unsigned char>& pixels,int& w,int& h,uint64_t& seq);
private:
 static void* thread_entry(void*);
 void decode();
 mutable std::mutex lock_;
 pthread_t worker_{};
 bool joined_=true;
 std::atomic<bool> quit_{false},paused_{false};
 PlaybackState state_;
 std::string path_;
 bool remote_=false;
 std::vector<unsigned char> pixels_;
 uint64_t sequence_=0;
};
}
