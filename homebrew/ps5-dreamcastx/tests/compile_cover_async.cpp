// PS5DreamcastX cover decoder regression test, no PS5 SDK required.
// A fake image decoder sleeps to prove it never executes in the UI thread.
// The worker must crop/scale to <=640 and GPU uploads must be on the UI thread.
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <thread>
using u8=uint8_t;
using ImTextureID=void*;
static std::thread::id uiThread;
static std::atomic<int> decoded{0}, uploaded{0};
static std::atomic<bool> wronglyDecodedOnUI{false},wronglyUploadedOffUI{false};
static u8* loadImage(const std::string&,int& width,int& height) {
    if(std::this_thread::get_id()==uiThread) wronglyDecodedOnUI=true;
    std::this_thread::sleep_for(std::chrono::milliseconds(35));
    width=1280;height=720;
    const size_t size=(size_t)width*height*4;
    u8* pixels=(u8*)std::malloc(size);
    assert(pixels);
    std::memset(pixels,127,size);
    decoded++;
    return pixels;
}
struct MockDriver {
    std::mutex mutex;
    std::map<std::string,ImTextureID> resident;
    ImTextureID getTexture(const std::string& path){
        std::lock_guard<std::mutex> lock(mutex);
        auto it=resident.find(path);
        return it==resident.end()?nullptr:it->second;
    }
    ImTextureID updateTextureAndAspectRatio(const std::string& path,const u8* data,
                                             int width,int height,bool){
        if(std::this_thread::get_id()!=uiThread) wronglyUploadedOffUI=true;
        assert(data&&width>0&&height>0&&width<=640&&height<=640);
        assert(width==640&&height==360);
        std::lock_guard<std::mutex> lock(mutex);
        ImTextureID result=reinterpret_cast<ImTextureID>((uintptr_t)(++uploaded+10));
        resident[path]=result;
        return result;
    }
};
static MockDriver realDriver;
static MockDriver* imguiDriver=&realDriver;
#define USE_PS5 1
#include "../src/dreamcastx_cover_async.inc"
int main(){
    uiThread=std::this_thread::get_id();
    using namespace dreamcastx_cover_async;
    assert(isCoverPath("/data/PS5DreamcastX/covers/Sonic.png"));
    assert(isCoverPath("/data/PS5DreamcastX/data/boxart/Sonic.png"));
    assert(!isCoverPath("/data/PS5DreamcastX/games/Sonic.gdi"));
    const std::string img="/data/PS5DreamcastX/covers/Sonic.png";
    int budget=0;
    auto t0=std::chrono::steady_clock::now();
    assert(texture(img,false,budget)==nullptr);
    assert(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-t0).count()<200);
    assert(budget==0 && !wronglyDecodedOnUI);
    ImTextureID id=nullptr;
    for(int i=0;i<300 && !id;i++){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        id=texture(img,false,budget);
    }
    assert(id!=nullptr);
    assert(budget==1&&uploaded==1&&decoded==1);
    assert(!wronglyUploadedOffUI&&!wronglyDecodedOnUI);
    // Repeat requests must use the graphics cache without decoding again.
    for(int i=0;i<25;i++)assert(texture(img,false,budget)==id);
    assert(decoded==1);
    // Additional decoded images obey the 2-texture-per-frame upload budget.
    const std::string second="/data/PS5DreamcastX/covers/Dino.png";
    const std::string third="/data/PS5DreamcastX/covers/Crazy.png";
    assert(texture(second,false,budget)==nullptr);
    assert(texture(third,false,budget)==nullptr);
    for(int i=0;i<300 && decoded<3;i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    assert(decoded==3);
    assert(texture(second,false,budget)!=nullptr);
    assert(budget==2);
    assert(texture(third,false,budget)==nullptr);
    assert(budget==2);
    budget=0;
    assert(texture(third,false,budget)!=nullptr);
    assert(budget==1);
    assert(!wronglyUploadedOffUI&&!wronglyDecodedOnUI);
}
