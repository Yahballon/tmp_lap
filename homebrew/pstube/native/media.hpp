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
 int width=0,height=0; double seconds=0,duration=0; bool seeking=false;
 std::string message;
};
class MediaPlayer {
public:
 MediaPlayer()=default;~MediaPlayer();
 bool open(const std::string& path);
 bool openStream(const std::string& url);
 bool openAdaptiveStream(const std::string& videoUrl,const std::string& audioUrl);
 bool openAdaptiveFiles(const std::string& videoPath,const std::string& audioPath);
 void close();
 void pause();
 // Request a relative seek; the FFmpeg worker performs all decoder operations.
 bool seekRelative(int deltaSeconds);
 PlaybackState snapshot() const;
 bool pull(std::vector<unsigned char>& pixels,int& w,int& h,uint64_t& seq);
private:
 static void* thread_entry(void*);
 void decode();
 mutable std::mutex lock_;
 pthread_t worker_{};
 bool joined_=true;
 std::atomic<bool> quit_{false},paused_{false};
 std::atomic<int64_t> seekDeltaUs_{0};
 PlaybackState state_;
 std::string path_,audioPath_;
 bool remote_=false;
 std::vector<unsigned char> pixels_;
 uint64_t sequence_=0;
};
}
