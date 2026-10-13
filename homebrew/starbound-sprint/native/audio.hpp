// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <pthread.h>
namespace sprint_audio {
enum class Sfx : int {
    jump, double_jump, crystal, stomp, hurt, spring,
    menu, pause, clear, death, victory, checkpoint, count
};
class Engine {
public:
    Engine()=default;
    ~Engine(){stop();}
    bool start();
    void stop();
    void play(Sfx effect);
    void set_scene(int stage,bool paused);
    bool active() const {return running_.load();}
    void mute(bool flag){muted_.store(flag);}
    bool muted() const {return muted_.load();}
private:
    struct Voice { float phase=0, pitch=0, age=0, length=0, gain=0, glide=0; int wave=0; bool active=false; };
    static constexpr int kBlock=256;
    std::array<Voice,16> voices_{};
    std::atomic<bool> running_{false},stop_{false},muted_{false},paused_{false};
    std::atomic<int> events_{0},scene_{0};
    pthread_t thread_{};
    bool threaded_=false;
    void* library_=nullptr;
    int port_=-1;
    using Init=int(*)();
    using Open=int(*)(int,int,int,int,int,int);
    using Output=int(*)(int,const void*);
    using Close=int(*)(int);
    Output output_=nullptr;
    Close close_=nullptr;
    static void* thread_start(void* self);
    void thread_loop();
    void note(int note,float length,float gain,int wave,float glide=0.f);
    void trigger(Sfx s);
    void music_tick();
};
} 
