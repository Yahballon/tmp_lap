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
bool load_backgrounds(Renderer& renderer,std::uint32_t& texture){
    constexpr int kWidth=256,kHeight=43,kBytes=kWidth*kHeight*4;
    constexpr const char* paths[]={
       "/app0/assets/starbound-backgrounds.rgba",
       "/data/homebrew/PPSA99641/assets/starbound-backgrounds.rgba"
    };
    std::vector<std::uint8_t> pixels(kBytes);
    bool valid=false;
    for(const char* path:paths){
        FILE* f=fopen(path,"rb");if(!f)continue;
        if(fseek(f,0,SEEK_END)!=0){fclose(f);continue;}
        long length=ftell(f);rewind(f);
        if(length==kBytes)valid=fread(pixels.data(),1,kBytes,f)==kBytes;
        fclose(f);
        if(valid)break;
    }
    if(!valid)return false;
    texture=renderer.batch().create_texture(kWidth,kHeight,pixels.data());
    if(!texture)return false;
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
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
void render_world(DrawList& ui,const sprint::Game& game,std::uint32_t atlas,std::uint32_t background,int frame){
    (void)background;
    const float cam=game.camera,t=game.elapsed;
    const int theme=game.tileset_theme();
    const Color white=tint(0xf6fcff);
    const unsigned night[4]={0x09142f,0x100b31,0x211329,0x13233f};
    ui.gradient_rect({0,145,1920,818},0,tint(night[theme]),tint(0x3c82b9));
    // High-resolution native geometry replaces enlarged 16-pixel background sprites.
    const float moonX=700.f-cam*.032f;
    ui.circle(moonX,285,145,tint(0x7dccff,.09f));
    ui.circle(moonX,285,112,tint(0xb9e8ff,.17f));
    ui.circle(moonX,285,86,tint(0xdcecff,.94f));
    ui.circle(moonX-22,265,24,tint(0x88b9dc,.26f));
    ui.circle(moonX+35,302,18,tint(0x88b9dc,.20f));
    for(int i=0;i<115;++i){
        float sx=std::fmod(i*193.3f-cam*.018f+23000.f,2020.f)-30.f;
        float sy=167.f+std::fmod(i*59.7f,430.f);
        float glow=.36f+.35f*std::sin(t*(game.progress.reduced_motion?0.f:1.8f)+i);
        ui.circle(sx,sy,1.3f+(i%4)*.48f,tint(0xecfcff,glow));
    }
    // Cloud banks in three distinct parallax planes.
    for(int layer=0;layer<3;++layer)for(int i=0;i<19;++i){
        float speed=.07f+layer*.11f;
        float x=std::fmod(i*193.f-cam*speed+6000.f,3650.f)-350.f;
        float y=354.f+layer*114.f+(i*43%91);
        Color color=tint(layer==0?0x8eacdb:layer==1?0x648fb8:0x4775a0,.21f+layer*.03f);
        ui.circle(x,y,84+layer*18,color);
        ui.circle(x+90,y-27,107+layer*11,color);
        ui.circle(x+185,y+4,79+layer*15,color);
    }
    // Far-off castle towers and dramatic floating islands.
    for(int i=0;i<12;++i){
        float x=std::fmod(170.f+i*332.f-cam*.19f+9400.f,4180.f)-310.f;
        float y=605.f+(i*37%105);
        ui.triangle({x-60,y-290,220,310},tint(0x183e69,.55f));
        ui.rounded_rect({x+25,y-195,44,210},5,tint(0x1f4779,.65f));
        ui.triangle({x+13,y-230,68,65},tint(0x245084,.77f));
        ui.rounded_rect({x+41,y-135,10,28},4,tint(0xffd8ab,.38f));
    }
    for(int i=0;i<8;++i){
        float x=std::fmod(i*470.f-cam*.29f+5200.f,3900.f)-300.f;
        float y=570.f+(i*59%130);
        ui.triangle({x,y,330,155},tint(0x1b3556,.78f));
        ui.rounded_rect({x+37,y-6,254,23},10,tint(0x4a9379,.68f));
        for(int j=0;j<7;++j)
            ui.line(x+40+j*39,y+15,x+37+j*40,y+47+(j%3)*12,2,tint(0x63caad,.39f));
    }
    // Waterfalls with animated light ripples (geometry stays crisp at 1080p).
    for(int i=0;i<4;++i){
        float x=std::fmod(760+i*730.f-cam*.38f+9200.f,4400.f)-100.f;
        float y=630.f+(i%2)*100;
        ui.rounded_rect({x,y,71,270},18,tint(0x4a9fd8,.33f));
        for(int k=0;k<6;++k)ui.line(x+7+k*10,y+8,x+12+k*9,y+262,2,
                  tint(0xd0f9ff,.16f+.09f*std::sin(t*3+k+i)));
    }
    ui.gradient_rect({0,748,1920,205},0,tint(0x145474,.06f),tint(0x173c57,.56f));
    // Interactive terrain is built from the REAL physics tile map, not baked into the backdrop.
    ui.push_clip({45,151,1830,804});
    const int first=std::max(0,(int)(cam/sprint::kTile)-2);
    const int last=std::min(sprint::kColumns-1,first+35);
    auto grass=[&ui,theme](float x,float y,float width){
        const Color moss=tint(theme==1?0x71e5eb:theme==2?0xe9aa62:0x6de8a0);
        ui.rounded_rect({x,y,width,12},3,tint(0x1e4c40));
        ui.rounded_rect({x,y-4,width,9},4,moss);
        ui.line(x+4,y+4,x+width-4,y+4,2,tint(0xc7ffa0,.51f));
        for(int i=0;i<(int)(width/13);++i){
            float px=x+6+i*13;
            ui.line(px,y-3,px+3,y-10-(i%3)*3,2,moss);
        }
    };
    for(int r=0;r<sprint::kRows;++r)for(int c=first;c<=last;++c){
        char tile=game.tiles[r][c];if(tile=='.')continue;
        float x=100.f+c*sprint::kTile-cam,y=153.f+r*sprint::kTile;
        if(tile=='#'){
            ui.rounded_rect({x,y,60,62},3,tint(0x223348));
            ui.rounded_rect({x+3,y+5,54,53},3,tint(theme==2?0x785064:0x3b485d));
            for(int k=0;k<3;++k)
                ui.line(x+6+(k*17+c*3)%43,y+12+k*16,
                        x+16+(k*19+c*3)%40,y+14+k*16,2,tint(0x8595b1,.36f));
            if(r==0||game.tiles[r-1][c]!='#')grass(x,y,60);
        }else if(tile=='='){
            ui.shadow({x,y,60,23},5,9,tint(0x000b29,.35f));
            ui.rounded_rect({x,y+3,60,24},6,tint(0x34485e));
            grass(x,y,60);
            for(int z=0;z<3;++z)ui.line(x+8+z*18,y+24,x+12+z*17,y+35+z*4,2,tint(0x67bb85,.7f));
        }else if(tile=='S'){
            ui.rounded_rect({x+5,y+33,50,17},6,tint(0xffaf64));
            ui.ring(x+30,y+23,13,4,tint(0xffe1a4,.8f));
        }else if(tile=='^'){
            for(int k=0;k<3;++k)ui.triangle({x+3+k*18,y+15,20,39},tint(0xe87f9a));
        }
    }
    // Wooden posts, glowing lanterns and floral vegetation.
    for(int c=5;c<sprint::kColumns;c+=17){
        float x=100.f+c*sprint::kTile-cam;
        if(!visible(x,120)||game.tiles[12][c]!='#')continue;
        float foot=153.f+12*sprint::kTile;
        ui.line(x+27,foot-8,x+27,foot-112,11,tint(0x6c4a42));
        ui.line(x+27,foot-93,x+82,foot-93,7,tint(0x946955));
        ui.line(x+78,foot-93,x+78,foot-58,3,tint(0xe0bd75));
        ui.circle(x+78,foot-50,21,tint(0xffc06c,.14f));
        ui.rounded_rect({x+63,foot-69,30,35},7,tint(0xb87847));
        ui.rounded_rect({x+69,foot-63,18,23},4,tint(0xffd18b,.86f));
    }
    for(const auto& plat:game.moving_platforms){
        float x=100+plat.pos.x-cam,y=153+plat.pos.y;
        if(!visible(x,180))continue;
        ui.shadow({x,y,180,37},10,17,tint(0x020a17,.45f));
        ui.rounded_rect({x,y,180,33},10,tint(0x344157));
        grass(x,y,180);
    }
    for(size_t i=0;i<game.pickups.size();++i){
        const auto& p=game.pickups[i];if(p.taken)continue;
        float x=100+p.pos.x-cam,y=153+p.pos.y;
        if(!visible(x,43))continue;
        float bob=game.progress.reduced_motion?0.f:std::sin(t*3.5f+i*.57f)*5.f;
        ui.circle(x+15,y+13+bob,19,tint(0x5debff,.14f));
        ui.star(x+15,y+13+bob,17,tint(0x36d8ff));
        ui.star(x+15,y+13+bob,10,tint(0xc4faff));
    }
    for(const auto& e:game.drones)if(e.alive){
        float x=100+e.pos.x-cam,y=153+e.pos.y;
        if(!visible(x,70))continue;
        float bob=game.progress.reduced_motion?0.f:std::sin(t*7+e.pos.x*.018f)*3.f;
        unsigned shell=(e.type%3==0)?0xdb6584:(e.type%3==1)?0x8ee47d:0xffa765;
        ui.circle(x+20,y+26+bob,27,tint(shell));
        ui.circle(x+9,y+20+bob,10,tint(0xffffff,.48f));
        ui.circle(x+29,y+20+bob,10,tint(0xffffff,.48f));
        ui.circle(x+11,y+22+bob,4,tint(0x10263e));
        ui.circle(x+29,y+22+bob,4,tint(0x10263e));
        if(e.type%3==2)ui.line(x+10,y+45+bob,x+35,y+45+bob,3,tint(0x1c253c));
    }
    float exitx=100+game.finish_x()-cam;
    if(visible(exitx,80)){
        ui.line(exitx+20,540,exitx+20,910,9,tint(0xe7d4a6));
        ui.star(exitx+20,548,45,tint(0xffd58c));
        ui.circle(exitx+20,548,57,tint(0xffd58c,.11f));
    }
    // Hero: large smooth vector character with readable silhouette and animated stride.
    if(game.invulnerable==0||(frame/6)%2==0){
        const float x=100+game.player.x-cam-14;
        const float y=153+game.player.y-23;
        const float stride=game.grounded?std::sin(t*12)*std::min(7.f,std::fabs(game.velocity.x)):1.f;
        ui.shadow({x+5,y+66,60,9},5,9,tint(0x07182c,.38f));
        ui.line(x+28,y+54,x+19+stride,y+70,8,tint(0x182a49));
        ui.line(x+39,y+54,x+50-stride,y+70,8,tint(0x182a49));
        ui.rounded_rect({x+13,y+29,45,36},12,tint(0x202b51));
        ui.triangle({x+(game.facing_right?8:37),y+29,47,42},tint(0xd94360));
        ui.circle(x+34,y+20,28,tint(0xeef5ff));
        ui.circle(x+35,y+22,22,tint(0x202c52));
        ui.circle(x+(game.facing_right?46:24),y+19,10,tint(0x68f0fa));
        ui.circle(x+(game.facing_right?48:26),y+17,5,tint(0xfcffff));
        for(int i=0;i<5;++i)ui.line(x+12+i*10,y+8,x+9+i*10,y-12-i%3*5,4,tint(0xe0eaff));
        if(!game.grounded&&game.jumps_remaining()==0)
            for(int i=0;i<4;++i)ui.star(x+15+i*13,y+69+std::sin(t*8+i)*6,4,tint(0x7af7fb,.7f));
    }
    ui.pop_clip();
}
} // namespace sprint_art
