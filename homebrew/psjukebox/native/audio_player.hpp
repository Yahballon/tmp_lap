// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <mutex>
#include <pthread.h>
#include <string>
namespace audio {
struct Status {
 bool opening=false, playing=false, paused=false, ended=false, seeking=false;
 double seconds=0, duration=0;
 std::string message;
};
class Player {
public:
 ~Player(){stop();}
 bool start(const std::string& path,double resume=0);
 void stop();
 void toggle_pause();
 bool seek(double seconds);
 bool skip(double delta);
 Status snapshot()const;
private:
 static void* entry(void* arg){static_cast<Player*>(arg)->decode();return nullptr;}
 void decode();
 mutable std::mutex mutex_;
 pthread_t thread_{};
 bool active_=false;
 std::atomic<bool> quit_{false},paused_{false};
 std::atomic<int64_t> seekMicroseconds_{-1};
 std::string path_;
 Status status_;
};
}
