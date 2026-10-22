// SPDX-License-Identifier: GPL-3.0-or-later
// Native PS5 stage selector/progress, not the original SurgeScript save format.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace surgepreview {
struct Progress {
    static constexpr int stages=4;
    int selected=0,unlocked=0;
    int checkpoint_stage=-1;
    float checkpoint_x=0,checkpoint_y=0;
    int best_score=0;

    void sanitize(){
        selected=std::clamp(selected,0,stages-1);
        unlocked=std::clamp(unlocked,0,stages-1);
        // QA build: selection can browse all four stages, independent of unlock.
        if(checkpoint_stage<0||checkpoint_stage>=stages ||
           !std::isfinite(checkpoint_x)||!std::isfinite(checkpoint_y)||
           std::abs(checkpoint_x)>100000.f||std::abs(checkpoint_y)>100000.f) {
            checkpoint_stage=-1;checkpoint_x=checkpoint_y=0;
        }
        best_score=std::clamp(best_score,0,10000000);
    }
    bool load(const std::string& path){
        FILE* f=std::fopen(path.c_str(),"rb");
        if(!f)return false;
        char magic[12]{};
        int n=std::fscanf(f,"%11s %d %d %d %f %f %d",magic,&selected,
                &unlocked,&checkpoint_stage,&checkpoint_x,&checkpoint_y,&best_score);
        std::fclose(f);
        if(n!=7||std::string(magic)!="OPS6"){
            *this=Progress{};return false;
        }
        sanitize();return true;
    }
    bool save(const std::string& path) const {
        Progress p=*this;p.sanitize();
        const std::string temp=path+".tmp";
        FILE* f=std::fopen(temp.c_str(),"wb");
        if(!f)return false;
        const int printed=std::fprintf(f,"OPS6 %d %d %d %.2f %.2f %d\n",
                p.selected,p.unlocked,p.checkpoint_stage,
                p.checkpoint_x,p.checkpoint_y,p.best_score);
        const bool flushed=std::fflush(f)==0;
        const bool closed=std::fclose(f)==0;
        const bool ok=printed>0 && flushed && closed;
        if(!ok){std::remove(temp.c_str());return false;}
        if(std::rename(temp.c_str(),path.c_str())!=0){
            std::remove(temp.c_str());return false;
        }
        return true;
    }
    void complete_stage(int index,int score){
        best_score=std::max(best_score,score);
        if(index>=0&&index<stages-1)
            unlocked=std::max(unlocked,index+1);
        checkpoint_stage=-1;
        selected=std::min(index+1,stages-1);
        sanitize();
    }
    bool checkpoint(int index,float x,float y){
        if(index<0||index>=stages||!std::isfinite(x)||!std::isfinite(y)||
           std::abs(x)>100000.f||std::abs(y)>100000.f)return false;
        checkpoint_stage=index;checkpoint_x=x;checkpoint_y=y;
        return true;
    }
};
} // namespace surgepreview
