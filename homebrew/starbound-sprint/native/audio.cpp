// SPDX-License-Identifier: GPL-3.0-or-later
// Native PS5 procedural audio. No commercial samples or copyrighted melodies.
#include "audio.hpp"
#include <dlfcn.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
namespace sprint_audio {
namespace {
constexpr int kRate=48000;
constexpr float kPi=3.14159265358979323846f;
float freq(int note){return 440.f*std::pow(2.f,(note-69)/12.f);}
float clip(float x){return std::max(-.92f,std::min(.92f,x));}
constexpr int kMelody[4][32]={
{72,-1,76,79,76,-1,74,72,69,-1,72,76,74,-1,67,-1,72,-1,76,79,81,79,76,-1,74,72,69,-1,67,69,72,-1},
{60,-1,63,67,70,-1,67,63,65,-1,68,72,70,-1,67,-1,60,-1,63,67,72,70,67,-1,65,63,60,-1,58,60,63,-1},
{57,-1,60,64,67,-1,64,60,62,-1,65,69,67,-1,64,-1,57,-1,60,64,69,67,64,-1,62,60,57,-1,55,57,60,-1},
{74,-1,77,81,79,-1,77,74,72,-1,74,77,81,-1,79,-1,74,-1,77,81,84,81,79,-1,77,74,72,-1,69,72,74,-1}
};
constexpr int kBass[4][8]={{48,48,45,43,48,48,45,43},{48,46,44,41,48,46,44,41},{45,45,41,43,45,45,41,43},{50,50,46,43,50,50,46,43}};
}
bool Engine::start(){
    if(running_)return true;
    library_=dlopen("libSceAudioOut.sprx",RTLD_LAZY);
    if(!library_)return false;
    auto init=reinterpret_cast<Init>(dlsym(library_,"sceAudioOutInit"));
    auto open=reinterpret_cast<Open>(dlsym(library_,"sceAudioOutOpen"));
    output_=reinterpret_cast<Output>(dlsym(library_,"sceAudioOutOutput"));
    close_=reinterpret_cast<Close>(dlsym(library_,"sceAudioOutClose"));
    if(!init||!open||!output_||!close_){dlclose(library_);library_=nullptr;return false;}
    init();
    // PS5 AudioOut: default user=0xFF, main port=0, index=0, 256 stereo S16 @ 48k.
    port_=open(0xFF,0,0,kBlock,kRate,1);
    if(port_<0){dlclose(library_);library_=nullptr;return false;}
    stop_=false;running_=true;
    if(pthread_create(&thread_,nullptr,&Engine::thread_start,this)!=0){
        running_=false;close_(port_);port_=-1;dlclose(library_);library_=nullptr;return false;
    }
    threaded_=true;
    return true;
}
void Engine::stop(){
    stop_=true;
    if(threaded_){pthread_join(thread_,nullptr);threaded_=false;}
    running_=false;
    if(port_>=0&&close_)close_(port_);
    port_=-1;
    if(library_){dlclose(library_);library_=nullptr;}
}
void Engine::play(Sfx s){
    int index=(int)s;
    if(index>=0&&index<(int)Sfx::count)events_.fetch_or(1<<index);
}
void Engine::set_scene(int stage,bool pause){scene_=std::max(0,std::min(7,stage));paused_=pause;}
void* Engine::thread_start(void* ptr){static_cast<Engine*>(ptr)->thread_loop();return nullptr;}
void Engine::note(int pitch,float length,float gain,int wave,float glide){
    for(auto& v:voices_)if(!v.active){
        v={0.f,freq(pitch),0.f,length,gain,glide,wave,true};return;
    }
}
void Engine::trigger(Sfx s){
    switch(s){
    case Sfx::jump: note(71,.11f,.26f,0,110.f);note(83,.075f,.13f,2);break;
    case Sfx::double_jump:note(76,.12f,.24f,1,260.f);note(88,.19f,.19f,2);break;
    case Sfx::crystal:note(86,.13f,.20f,2);note(93,.20f,.18f,2);break;
    case Sfx::stomp:note(50,.12f,.26f,1,-150.f);break;
    case Sfx::hurt:note(45,.25f,.29f,1,-240.f);break;
    case Sfx::spring:note(66,.26f,.30f,2,560.f);break;
    case Sfx::menu:note(78,.08f,.18f,2);break;
    case Sfx::pause:note(62,.09f,.16f,2);break;
    case Sfx::clear:for(int i=0;i<3;++i)note(72+i*4,.52f,.13f,2);break;
    case Sfx::death:note(44,.60f,.28f,1,-190.f);break;
    case Sfx::victory:for(int i=0;i<4;++i)note(72+i*4,.90f,.14f,2);break;
    case Sfx::checkpoint:note(79,.2f,.20f,2);note(86,.25f,.13f,2);break;
    default:break;
    }
}
void Engine::thread_loop(){
    alignas(64) std::array<std::int16_t,kBlock*2> samples{};
    std::uint64_t index=0;int lastScene=-1;
    constexpr int stepSamples=12000; // Four melodic notes per second.
    while(!stop_){
        int eventBits=events_.exchange(0);
        if(!muted_)for(int i=0;i<(int)Sfx::count;++i)
            if(eventBits&(1<<i))trigger((Sfx)i);
        if(scene_.load()!=lastScene){lastScene=scene_.load();index=0;}
        for(int i=0;i<kBlock;++i,++index){
            const bool playing=!paused_.load()&&!muted_.load();
            if(playing&&index%stepSamples==0){
                int step=(int)(index/stepSamples)%32,world=lastScene%4;
                int notePitch=kMelody[world][step];
                if(notePitch>=0)note(notePitch,.18f,.065f,2);
                if(step%4==0)note(kBass[world][(step/4)%8],.31f,.07f,0);
            }
            float mix=0.f;
            for(auto& v:voices_)if(v.active){
                float frequency=std::max(30.f,v.pitch+v.age*v.glide);
                v.phase+=frequency/kRate;
                if(v.phase>=1.f)v.phase-=std::floor(v.phase);
                float sample=v.wave==0?std::sin(v.phase*2.f*kPi):v.wave==1?(v.phase<.5f?1.f:-1.f):4.f*std::fabs(v.phase-.5f)-1.f;
                float envelope=std::min(1.f,v.age*90.f)*std::max(0.f,1.f-v.age/v.length);
                mix+=sample*envelope*v.gain;
                v.age+=1.f/kRate;
                if(v.age>=v.length)v.active=false;
            }
            auto out=(std::int16_t)std::lrint(clip(muted_?0.f:mix)*32767.f);
            samples[i*2]=out;samples[i*2+1]=out;
        }
        if(output_(port_,samples.data())<0)break;
    }
    running_=false;
}
}
