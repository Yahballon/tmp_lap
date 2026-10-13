// SPDX-License-Identifier: GPL-3.0-or-later
#include "art_renderer.hpp"
#include <GL/glcorearb.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

namespace sprint_art {
using namespace hui::gfx;
namespace {
constexpr int kAtlasBytes=kAtlasPixels*kAtlasPixels*4;
constexpr float kScreenX=100.f,kScreenY=153.f;
const Color kWhite=Color::rgb(0xffffff);
constexpr std::array<unsigned,8> kSky={
  0x225986,0x100d42,0x613143,0x437fa8,0x0e6573,0x1d1759,0x9a3d3f,0x344e9e
};
constexpr std::array<unsigned,8> kHorizon={
  0x91b9d9,0x345c8d,0xf39b76,0xbddbe9,0x78c8bf,0x72609d,0xffbf71,0xa8d8ff
};
Color tint(unsigned rgb,float opacity=1.f){return Color::rgb(rgb,opacity);}
bool visible(float x,float w=60.f){return x+w>=43.f&&x<=1878.f;}
float fract(float x){return x-std::floor(x);}
}
bool load_atlas(Renderer& renderer,std::uint32_t& texture){
    constexpr const char* locations[]={
        "/app0/assets/starbound-sprites.rgba",
        "/data/homebrew/PPSA99641/assets/starbound-sprites.rgba"
    };
    std::vector<std::uint8_t> pixels;
    for(const char* path:locations){
        FILE* file=fopen(path,"rb");if(!file)continue;
        if(fseek(file,0,SEEK_END)!=0){fclose(file);continue;}
        long len=ftell(file);rewind(file);
        if(len!=kAtlasBytes){fclose(file);continue;}
        pixels.resize(kAtlasBytes);
        const size_t got=fread(pixels.data(),1,kAtlasBytes,file);
        fclose(file);
        if(got==kAtlasBytes)break;
        pixels.clear();
    }
    if(pixels.size()!=kAtlasBytes)return false;
    texture=renderer.batch().create_texture(kAtlasPixels,kAtlasPixels,pixels.data());
    if(!texture)return false;
    // Pixel art is deliberately not blurred with bilinear sampling.
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D,0);
    return true;
}
void release_atlas(std::uint32_t& texture){
    if(texture){GLuint id=texture;glDeleteTextures(1,&id);texture=0;}
}
void sprite(DrawList& draw,std::uint32_t texture,int index,Rect rect,bool facing_right,float alpha){
    if(!texture||index<0||index>=64)return;
    constexpr float step=1.f/8.f;
    const float left=(index%8)*step, top=(index/8)*step;
    const float pad=0.00001f;
    Rect uv{facing_right?left+pad:left+step-pad,top+pad,
            facing_right?left+step-pad:left+pad,top+step-pad};
    draw.image(texture,rect,uv,kWhite.with_alpha(alpha));
}
void render_world(DrawList& ui,const sprint::Game& game,std::uint32_t atlas,int frame){
    int theme=game.tileset_theme();
    const float cam=game.camera;
    ui.gradient_rect({0,0,1920,1080},0,tint(kSky[game.stage]),tint(0x081223));
    // Three parallax scenery layers; generated geometrically, not scaled full-screen bitmaps.
    // Slow moving cloud halos, floating distant ruins and sparkling stars.
    for(int i=0;i<80;++i){
        float px=fract(i*.6180339887f+.11f)*2200.f;
        float x=std::fmod(px-cam*.11f+5000.f,2200.f)-100.f;
        float y=146.f+fract(i*.37123f)*660.f;
        float pulse=.45f+.35f*std::sin(game.elapsed*1.4f+i*1.83f);
        ui.circle(x,y,1.5f+i%3,tint(0xe2fbff,pulse));
    }
    for(int i=0;i<17;++i){
        float x=std::fmod(180.f+i*198.f-cam*.16f+4400.f,3400.f)-260.f;
        float y=550.f+(i*71%160);
        ui.triangle({x,y,400,410},tint(kHorizon[game.stage],.18f));
        ui.circle(x+215.f,y+215.f,76.f,tint(kHorizon[game.stage],.10f));
    }
    for(int i=0;i<12;++i){
        float x=std::fmod(i*305.f-cam*.30f+7000.f,4000.f)-240.f;
        float y=750.f+(i%4)*18.f;
        ui.rounded_rect({x,y,245,240},100,tint(kHorizon[game.stage],.13f));
    }
    ui.push_clip({43,150,1835,795});
    int first=std::max(0,static_cast<int>(cam/sprint::kTile)-3);
    int last=std::min(sprint::kColumns-1,first+36);
    // Only render map cells inside camera's view (always fewer than 500).
    for(int r=0;r<sprint::kRows;++r)for(int c=first;c<=last;++c){
        char type=game.tiles[r][c];if(type=='.')continue;
        float x=kScreenX+c*sprint::kTile-cam;
        float y=kScreenY+r*sprint::kTile;
        int base=24+theme*8;
        if(type=='#'){
            int tile=base+(r==sprint::kRows-1?((c+game.stage)%3):((c*3+r)%8));
            sprite(ui,atlas,tile,{x,y,60,60});
        }else if(type=='='){
            sprite(ui,atlas,base+(2+((c+game.stage)%3)),{x,y,60,26});
            ui.rounded_rect({x+3,y+21,54,4},2,tint(0x07121f,.28f));
        }else if(type=='S'){
            sprite(ui,atlas,62,{x+4,y+12,52,50});
            ui.circle(x+30,y+14,8.f+4.f*std::sin(game.elapsed*4.f),tint(0xffcc6f,.45f));
        }else if(type=='^'){
            sprite(ui,atlas,42,{x+4,y+13,52,52});
            ui.triangle({x+12,y+6,35,34},tint(0xff637d,.8f));
        }
    }
    // Frame-based scenery draws from the approved concept art atlas.
    for(int i=0;i<sprint::kColumns/8;++i){
        int col=i*8+5;
        float x=kScreenX+col*sprint::kTile-cam;
        if(!visible(x,170.f))continue;
        if(game.tiles[12][col]=='#' && i%4!=1){
            float size=(i%3==0)?150.f:112.f;
            sprite(ui,atlas,56+(i+theme)%8,{x,153+12*sprint::kTile-size+18,size,size});
        }
    }
    for(const auto& p:game.moving_platforms){
        float x=kScreenX+p.pos.x-cam,y=kScreenY+p.pos.y;
        if(!visible(x,180.f))continue;
        ui.shadow({x,y,180,27},8,10,tint(0x00142b,.24f));
        for(int i=0;i<3;++i)sprite(ui,atlas,25+theme*8,{x+i*60.f,y,60,36});
        ui.line(x+8,y+33,x+172,y+33,2,tint(0xf4ecb4,.65f));
    }
    // Collectible animations and pulsing pickup rings.
    for(size_t i=0;i<game.pickups.size();++i){
        const auto& p=game.pickups[i];if(p.taken)continue;
        float x=kScreenX+p.pos.x-cam,y=kScreenY+p.pos.y;
        if(!visible(x,40.f))continue;
        float bounce=std::sin(game.elapsed*3.9f+static_cast<float>(i)*.9f)*7.f;
        int index=(i%9==0)?17:(i%17==0)?23:16;
        ui.circle(x+17,y+14+bounce,19,tint(0x5bf5ff,.16f));
        sprite(ui,atlas,index,{x,y-4+bounce,34,34});
        if(i%4==0)ui.star(x+37,y+bounce,4.f+2.f*std::sin(game.elapsed*5+i),tint(0xffe8af));
    }
    // Three animation behaviors: land patrol, flying bob, hazard rover.
    for(const auto& e:game.drones){
        if(!e.alive)continue;
        float x=kScreenX+e.pos.x-cam,y=kScreenY+e.pos.y;
        if(!visible(x,66))continue;
        float bob=std::sin(game.elapsed*8.f+e.pos.x*.02f)*3.f;
        int frameId=8+(e.type%8);
        if(e.type==3||e.type==7)bob*=2.f;
        sprite(ui,atlas,frameId,{x-6,y-8+bob,60,60},e.direction>=0);
        if(e.type==1)ui.circle(x+22,y+7,9,tint(0xffe57c,.24f));
    }
    const float exitX=kScreenX+game.finish_x()-cam;
    if(visible(exitX,100.f)){
        sprite(ui,atlas,60,{exitX-25,535,116,380});
        ui.ring(exitX+29,618,23,5,tint(0x92fff4,.9f));
    }
    // Animated player sprite selection, including special airborne states and walk cycle.
    if(game.invulnerable==0||(frame/5)%2==0){
        int pose=0;
        if(!game.grounded)pose=(game.velocity.y<0)?(game.jumps_remaining()==0?6:6):7;
        else if(std::fabs(game.velocity.x)>1.f)
            pose=1+((frame/(std::fabs(game.velocity.x)>5.5f?5:9))%5);
        float x=kScreenX+game.player.x-cam-9,y=kScreenY+game.player.y-9;
        if(game.jumps_remaining()==0&&!game.grounded){
            ui.circle(x+35,y+52,33,tint(0x00ddfa,.16f));
            for(int i=0;i<5;++i){
                float angle=game.elapsed*7.f+i*6.28318f/5.f;
                ui.star(x+33+std::cos(angle)*30.f,y+41+std::sin(angle)*27.f,4.f,tint(0x7bfff3,.75f));
            }
        }
        sprite(ui,atlas,pose,{x,y,68,73},game.facing_right);
    }
    ui.pop_clip();
}
} // namespace sprint_art
