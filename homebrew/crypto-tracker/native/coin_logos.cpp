// SPDX-License-Identifier: GPL-3.0-or-later
#include "coin_logos.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>
namespace tracker {
namespace {
struct IconEntry { const char* symbol; unsigned tile; };
#include "coin_atlas.inc"
constexpr int kAtlasWidth = 1024;
constexpr int kAtlasHeight = 2048;
constexpr int kTile = 64;
bool file_read(const char* path,std::vector<std::uint8_t>& bytes) {
    FILE* fp=std::fopen(path,"rb");
    if(!fp)return false;
    if(std::fseek(fp,0,SEEK_END)!=0){std::fclose(fp);return false;}
    const long n=std::ftell(fp);
    if(n!=static_cast<long>(kAtlasWidth*kAtlasHeight*4)){
        std::fclose(fp);return false;
    }
    std::rewind(fp);
    bytes.resize(static_cast<std::size_t>(n));
    bool result=std::fread(bytes.data(),1,bytes.size(),fp)==bytes.size();
    std::fclose(fp);return result;
}
unsigned hash(std::string_view s) {
    unsigned n=2166136261u;
    for(unsigned char c:s)n=(n^c)*16777619u;
    return n;
}
}
bool CoinLogos::init(hui::gfx::Renderer& renderer) {
    std::vector<std::uint8_t> data;
    // Support both app0 mounts and direct /data/homebrew folder launches.
    constexpr const char* locations[] = {
        "/app0/assets/coin-atlas.rgba",
        "/data/homebrew/PPSA99442/assets/coin-atlas.rgba",
        "/mnt/sandbox/PPSA99442_000/app0/assets/coin-atlas.rgba"
    };
    for(const auto* p:locations)if(file_read(p,data))break;
    if(data.empty())return false;
    texture_=renderer.batch().create_texture(kAtlasWidth,kAtlasHeight,data.data());
    return texture_!=0;
}
int CoinLogos::index_of(std::string_view sym) const {
    std::string s;
    for(unsigned char c:sym) {
        if(c>=128 || s.size()>=24)return -1;
        s.push_back(static_cast<char>(std::tolower(c)));
    }
    // This is a small, build-generated table. Avoid dynamic allocation per frame.
    for(unsigned i=0;i<kIconCount;i++)
        if(s==kIconEntries[i].symbol)return static_cast<int>(kIconEntries[i].tile);
    return -1;
}
void CoinLogos::draw(hui::gfx::DrawList& ui,
    const hui::gfx::Font& font,std::uint32_t fontAtlas,
    const Coin& coin,float x,float y,float diameter) const {
    const float radius=diameter*.5f;
    const int index=index_of(coin.symbol);
    if(texture_ && index>=0) {
        const float u=float(index%16*kTile)/kAtlasWidth;
        const float v=float(index/16*kTile)/kAtlasHeight;
        const float uw=float(kTile)/kAtlasWidth;
        const float vh=float(kTile)/kAtlasHeight;
        ui.image(texture_,{x-radius,y-radius,diameter,diameter},
                 {u,v,uw,vh},hui::gfx::Color::rgb(0xffffff),radius);
        return;
    }
    // New tokens without a CC0 icon get a deterministic, unique ticker badge.
    const unsigned value=hash(coin.id);
    const unsigned r=45+(value&0x7f);
    const unsigned g=55+((value>>8)&0x7f);
    const unsigned b=65+((value>>16)&0x7f);
    ui.circle(x,y,radius,hui::gfx::Color{float(r)/255.0f,float(g)/255.0f,float(b)/255.0f,1});
    std::string text=coin.symbol.substr(0,coin.symbol.size()>3?2:3);
    ui.text(font,fontAtlas,text,x,y+diameter*.13f,diameter*.28f,
            hui::gfx::Color::rgb(0xffffff),hui::gfx::Align::center);
}
} // namespace tracker
