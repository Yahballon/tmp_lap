// SPDX-License-Identifier: GPL-3.0-or-later
// Original Open Surge SFX / music through ps5-homebrew-ui native sceAudioOut.
// Decoding is on the main thread ahead of the 48k/256-frame audio thread.
#pragma once
#include "audio/mixer.hpp"
#include "audio/music.hpp"
#include "audio/stream_ring.hpp"
#include "audio/wav.hpp"
#include "platform/ps5/audio_out.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace surgepreview {

enum class Sound : std::size_t {
    Jump, Collectible, CollectibleLoss, Spring, Roll, Charge, ChargingUp,
    Release, Brake, Damaged, Death, Destroy, Spikes, Shot, Endsign, Checkpoint,
    Count,
};

class GameAudio {
public:
    GameAudio()=default;
    GameAudio(const GameAudio&)=delete;
    GameAudio& operator=(const GameAudio&)=delete;
    ~GameAudio(){shutdown();}

    // Audio is optional: a failure should never break the stable PS5 renderer.
    bool init(const std::string& root){
        if(root.size()<5||root.substr(root.size()-5)!="game/"){
            status_="AUDIO NO GAME ROOT";return false;
        }
        const std::string base=root.substr(0,root.size()-5)+"audio-cache/";
        static constexpr const char* names[]={
            "jump","collectible","collectible_loss","spring","roll","charge",
            "charging_up","release","brake","damaged","death","destroy",
            "spikes","shot","endsign","checkpoint",
        };
        static_assert(sizeof(names)/sizeof(*names)==
            static_cast<std::size_t>(Sound::Count));
        loaded_=0;
        for(std::size_t i=0;i<clips_.size();++i){
            const std::string wav=read(base+names[i]+".wav",4*1024*1024);
            if(wav.empty())continue;
            auto decoded=hui::audio::decode_wav(wav);
            if(!decoded.ok()||decoded.frames==0)continue;
            clips_[i].buffer=std::move(decoded.samples);
            clips_[i].clip.samples=clips_[i].buffer.data();
            clips_[i].clip.frames=decoded.frames;
            ++loaded_;
        }
        const auto data=read(base+"level.ogg",32*1024*1024);
        if(!data.empty()) {
            const std::string error=track_.open(data);
            music_ready_=error.empty();
            if(!music_ready_)status_="OGG VORBIS ERROR: "+error;
        }
        mixer_.attach_stream(0,&ring_);
        mixer_.set_stream_gain(0,0.32f,0.0f);
        mixer_.set_master_gain(0.90f);
        if(music_ready_)pump();
        opened_=output_.start(mixer_);
        if(!opened_){
            status_="SCE_AUDIO_OUT_OPEN_FAILED";
            return false;
        }
        if(!music_ready_)status_="AUDIO_SFX_ONLY";
        else status_="AUDIO_ORIGINAL_SFX_AND_LEVEL_MUSIC";
        return true;
    }
    bool ready() const{return opened_;}
    bool music_ready() const{return music_ready_;}
    bool music_enabled() const{return music_enabled_;}
    int clips_loaded() const{return loaded_;}
    const std::string& status() const{return status_;}
    std::uint64_t output_errors() const{return output_.errors();}
    std::uint64_t underruns() const{return mixer_.stream_underruns();}
    void level_active(bool active){
        if(!opened_)return;
        active_=active;
        mixer_.set_stream_gain(0,(active_&&music_enabled_&&music_ready_)?0.32f:0.f,0.15f);
    }
    bool toggle_music(){
        music_enabled_=!music_enabled_;
        level_active(active_);
        return music_enabled_;
    }
    bool play(Sound name,float gain=0.8f){
        if(!opened_)return false;
        const auto& sound=clips_[static_cast<std::size_t>(name)];
        if(sound.clip.frames==0)return false;
        hui::audio::PlayParams p;
        p.bus=hui::audio::Bus::sfx;
        p.gain=std::clamp(gain,0.f,1.f);
        return mixer_.play_clip(&sound.clip,p);
    }
    void pump(){
        // Keep enough music decoded ahead to prevent underruns while the
        // original tile renderer costs variable CPU time.
        if(!music_ready_ || (!opened_ && ring_.available()>24000))return;
        for(int count=0;count<18 && ring_.available()<26000;++count){
            const int frames=int(std::min<std::size_t>(1024,ring_.space()));
            if(frames<=0)break;
            const int n=track_.decode(scratch_.data(),frames);
            if(n<=0)break;
            ring_.write(scratch_.data(),std::size_t(n));
        }
    }
    void shutdown(){
        if(opened_){output_.stop();opened_=false;}
    }

private:
    struct OwnedClip {
        std::vector<float> buffer;
        hui::audio::Clip clip;
    };
    static std::string read(const std::string& path,std::size_t max){
        FILE* f=std::fopen(path.c_str(),"rb");
        if(!f)return {};
        if(std::fseek(f,0,SEEK_END)){std::fclose(f);return {};}
        const long n=std::ftell(f);
        if(n<=0||static_cast<std::size_t>(n)>max){std::fclose(f);return {};}
        std::rewind(f);
        std::string data(static_cast<std::size_t>(n),'\0');
        const bool ok=std::fread(data.data(),1,data.size(),f)==data.size();
        std::fclose(f);
        return ok?data:std::string();
    }

    hui::audio::Mixer mixer_;
    hui::audio::StreamRing ring_{1u<<16};
    hui::audio::MusicTrack track_;
    hui::ps5::AudioOut output_;
    std::array<OwnedClip,static_cast<std::size_t>(Sound::Count)> clips_{};
    std::array<float,2048> scratch_{};
    std::string status_="AUDIO_NOT_INITIALIZED";
    int loaded_=0;
    bool music_ready_=false,opened_=false,active_=false,music_enabled_=true;
};
} // namespace surgepreview
