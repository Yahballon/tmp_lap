// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "bible.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>
namespace bible_tts {
struct WordMark {int timeMs=0;size_t begin=0,end=0;};
struct Snapshot {
 bool active=false,playing=false,neural=false;
 bible::Ref ref{};
 size_t wordBegin=0,wordEnd=0;
 std::string error;
};
size_t utf8Offset(const std::string& value,size_t characters);
bool nextVerse(const bible::Library& lib,bible::Ref& ref);
std::string neuralPreviewFilename(int language,bible::Ref ref);
const char* classicVoiceProfile(int language);
class Speaker {
public:
 ~Speaker(){stop();}
 Speaker(const Speaker&)=delete;
 Speaker& operator=(const Speaker&)=delete;
 Speaker()=default;
 bool start(const bible::Library& lib,bible::Ref from,int language,bool preferNeural=false);
 void stop();
 Snapshot snapshot() const;
 bool active() const;
 // Called by the eSpeak NG synthesis callback from the worker thread.
 int collect(const short* pcm,int frames,int words,const int* start,const int* length,const int* ms);
private:
 static void* entry(void* arg){static_cast<Speaker*>(arg)->run();return nullptr;}
 void run();
 void status(Snapshot value);
 mutable std::mutex mutex_;
 Snapshot status_{};
 const bible::Library* library_=nullptr;
 bible::Ref from_{};
 int language_=0;
 bool preferNeural_=false;
 bool robotPreset_=false;
 pthread_t thread_{};
 bool joined_=true;
 std::atomic<bool> cancelled_{false};
 std::vector<short> pcm_{};
 std::vector<WordMark> marks_{};
 std::string phrase_{};
};
}
