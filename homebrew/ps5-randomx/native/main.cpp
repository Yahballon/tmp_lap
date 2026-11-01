// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 RANDOMX BENCHMARK by YAHBALLON v0.1.0
// Real RandomX v1.2.1, light mode, interpreter; NOT a cryptocurrency miner.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#include "randomx.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <pthread.h>
#include <sys/stat.h>
using namespace hui::gfx;

namespace {
constexpr const char* kDataDir = "/data/ps5-randomx";
constexpr const char* kReport = "/data/ps5-randomx/benchmark.log";
constexpr const char* kKey = "PS5Coin benchmark - genesis-independent key - v0.1";
enum Stage : int { Ready=0, Preparing=1, Hashing=2, Completed=3, Stopped=4, Failed=5 };
struct Bench {
  std::atomic<int> stage{Ready};
  std::atomic<bool> stop{false};
  std::atomic<uint64_t> hashes{0}, elapsed_us{0};
  std::atomic<unsigned> sample{0};
  int seconds=30;
  pthread_t thread{};
  bool join_needed=false;
  char error[128]="";
  char last_hash[65]="";
  char report_status[96]="No result yet";
};
std::string readFile(const char* path) {
  FILE* f=std::fopen(path,"rb"); if(!f) return {};
  if(std::fseek(f,0,SEEK_END)!=0){std::fclose(f);return {};}
  const long n=std::ftell(f); if(n<=0 || n>8000000){std::fclose(f);return {};}
  std::rewind(f);std::string data(size_t(n),'\0');
  const size_t got=std::fread(data.data(),1,data.size(),f);
  std::fclose(f);return got==data.size()?data:std::string{};
}
void report(Bench* b) {
  mkdir(kDataDir,0777);
  FILE* f=std::fopen(kReport,"a");
  if(!f){std::snprintf(b->report_status,sizeof(b->report_status),"Cannot open %s",kReport);return;}
  const double elapsed=double(b->elapsed_us.load())/1000000.0;
  const uint64_t total=b->hashes.load();
  const double hps=elapsed>0?double(total)/elapsed:0.0;
  std::fprintf(f,"PS5 RANDOMX BENCHMARK by YAHBALLON v0.1.0\n");
  std::fprintf(f,"algorithm=RandomX-v1.2.1 mode=light-interpreter aes=software jit=off threads=1\n");
  std::fprintf(f,"requested_seconds=%d elapsed_seconds=%.3f hashes=%llu hashrate_hps=%.4f\n",
               b->seconds,elapsed,(unsigned long long)total,hps);
  std::fprintf(f,"last_hash=%s\nstatus=%s\n\n",b->last_hash,
               b->stage.load()==Completed?"completed":"interrupted");
  std::fclose(f);
  std::snprintf(b->report_status,sizeof(b->report_status),"Saved to %s",kReport);
}
void* runBench(void* context) {
  Bench* b=static_cast<Bench*>(context);
  b->stage.store(Preparing);
  randomx_cache* cache=randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
  if(!cache){std::snprintf(b->error,sizeof(b->error),"RandomX cache allocation failed (~256 MiB)");b->stage.store(Failed);return nullptr;}
  randomx_init_cache(cache,kKey,std::strlen(kKey));
  if(b->stop.load()){randomx_release_cache(cache);b->stage.store(Stopped);return nullptr;}
  randomx_vm* vm=randomx_create_vm(RANDOMX_FLAG_DEFAULT,cache,nullptr);
  if(!vm){randomx_release_cache(cache);std::snprintf(b->error,sizeof(b->error),"RandomX interpreter VM allocation failed");b->stage.store(Failed);return nullptr;}
  std::array<uint8_t,64> input{};
  std::memcpy(input.data(),"PS5RandomX-v0.1 PS5Coin independent bench",40);
  std::array<uint8_t,RANDOMX_HASH_SIZE> digest{};
  const uint64_t begin=hui::sys::monotonic_us();
  b->stage.store(Hashing);
  uint64_t nonce=0;
  while(!b->stop.load()){
    std::memcpy(input.data()+56,&nonce,sizeof(nonce));
    randomx_calculate_hash(vm,input.data(),input.size(),digest.data());
    ++nonce;
    b->hashes.store(nonce);
    const uint64_t elapsed=uint64_t(hui::sys::monotonic_us())-begin;
    b->elapsed_us.store(elapsed);
    if(elapsed>=uint64_t(b->seconds)*1000000ULL)break;
  }
  for(size_t i=0;i<digest.size();++i)
    std::snprintf(b->last_hash+2*i,3,"%02x",unsigned(digest[i]));
  b->stage.store(b->stop.load()?Stopped:Completed);
  randomx_destroy_vm(vm);
  randomx_release_cache(cache);
  report(b);
  return nullptr;
}
bool start(Bench& b,int seconds){
  if(b.join_needed)return false;
  b.seconds=seconds; b.hashes.store(0); b.elapsed_us.store(0);
  b.stop.store(false); b.error[0]=0; b.last_hash[0]=0;
  std::snprintf(b.report_status,sizeof(b.report_status),"Initializing RandomX cache...");
  b.stage.store(Preparing);
  if(pthread_create(&b.thread,nullptr,runBench,&b)!=0){
    std::snprintf(b.error,sizeof(b.error),"Cannot create worker thread");
    b.stage.store(Failed);return false;
  }
  b.join_needed=true;return true;
}
void reap(Bench& b){
  const int state=b.stage.load();
  if(b.join_needed&&(state==Completed||state==Stopped||state==Failed)){
    pthread_join(b.thread,nullptr);
    b.join_needed=false;
  }
}
Color color(unsigned hex,float a=1.f){return Color::rgb(hex,a);}
void tx(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,float size,Color c=color(0xf2f7ff),Align align=Align::left){
  d.text(f,a,s,x,y,size,c,align);
}
void panel(DrawList& d,float x,float y,float w,float h){
  d.bordered_rect({x,y,w,h},20,color(0x08162c),2,color(0x193a66));
}
const char* statusName(int s){
  switch(s){case Preparing:return "INITIALIZING CACHE";case Hashing:return "BENCHMARK RUNNING";
    case Completed:return "BENCHMARK COMPLETE";case Stopped:return "STOPPED";case Failed:return "ERROR";default:return "READY";}
}
}
int main(){
  hui::ps5::Display display; if(!display.open())hui::sys::park();
  hui::gfx::Renderer renderer; if(!renderer.init())hui::sys::park();
  std::string fontData=readFile("/app0/assets/fonts/inter-semibold.huifont");
  if(fontData.empty())fontData=readFile("/data/homebrew/PPSA99795/assets/fonts/inter-semibold.huifont");
  Font font; if(!font.load(fontData))hui::sys::park();
  const unsigned atlas=renderer.batch().create_font_texture(font);
  hui::ps5::Pad pad;pad.open();
  Bench b;int duration_index=1;constexpr int durations[]={10,30,60};
  unsigned prior=0;bool exit_requested=false;bool export_hint=false;
  for(;;){
    reap(b);
    std::array<hui::PadSample,64> samples{};
    const size_t count=pad.read(samples);
    unsigned buttons=0;
    for(size_t i=0;i<count;i++){
      if(!samples[i].connected)continue;
      buttons=samples[i].buttons;
      if(samples[i].left_x<68)buttons|=hui::pad_bits::kLeft;
      else if(samples[i].left_x>188)buttons|=hui::pad_bits::kRight;
    }
    const unsigned pressed=buttons&~prior;prior=buttons;
    const auto key=[&](unsigned bit){return (pressed&bit)!=0;};
    const int state=b.stage.load();
    const bool busy=state==Preparing||state==Hashing;
    if(!busy){
      if(key(hui::pad_bits::kLeft)||key(hui::pad_bits::kL1))duration_index=(duration_index+2)%3;
      if(key(hui::pad_bits::kRight)||key(hui::pad_bits::kR1))duration_index=(duration_index+1)%3;
      if(key(hui::pad_bits::kCross)){export_hint=false;start(b,durations[duration_index]);}
      if(key(hui::pad_bits::kTriangle)){b.hashes.store(0);b.elapsed_us.store(0);b.last_hash[0]=0;b.stage.store(Ready);export_hint=false;}
      if(key(hui::pad_bits::kCircle))exit_requested=true;
      if(key(hui::pad_bits::kSquare))export_hint=true;
    }else if(key(hui::pad_bits::kCircle)||key(hui::pad_bits::kOptions)){
      b.stop.store(true);
    }
    if(exit_requested && !b.join_needed)break;

    const uint64_t hashes=b.hashes.load(), elapsed=b.elapsed_us.load();
    const double hps=elapsed>0?double(hashes)*1000000.0/double(elapsed):0.0;
    const double seconds=double(elapsed)/1000000.0;
    const float progress=std::clamp(float(seconds/double(b.seconds)),0.f,1.f);
    char rate[80],time[80],total[80];
    std::snprintf(rate,sizeof(rate),"%.2f H/s",hps);
    std::snprintf(time,sizeof(time),"%.1f / %d sec",seconds,b.seconds);
    std::snprintf(total,sizeof(total),"%llu",(unsigned long long)hashes);
    DrawList d;
    d.gradient_rect({0,0,1920,1080},0,color(0x040915),color(0x0a1d35));
    d.circle(1770,250,290,color(0x0c3553,.20f));
    d.rounded_rect({0,0,1920,102},0,color(0x061225));
    d.rounded_rect({39,24,52,52},13,color(0x0761cc));
    tx(d,font,atlas,"RX",65,60,27,color(0xffffff),Align::center);
    tx(d,font,atlas,"PS5 RANDOMX BENCHMARK  by YAHBALLON",113,61,33);
    tx(d,font,atlas,"v0.1.0",1879,60,24,color(0xc9e4ff),Align::right);
    tx(d,font,atlas,"REAL RANDOMX PROOF-OF-WORK PERFORMANCE TEST",63,165,19,color(0x70bbfd));
    panel(d,60,190,1190,400);
    tx(d,font,atlas,statusName(state),97,257,30,color(busy?0x57dcff:state==Failed?0xff6488:0x69efbd));
    tx(d,font,atlas,rate,100,392,88);
    tx(d,font,atlas,"Single CPU worker  /  No JIT  /  Software AES  /  Light mode",103,455,23,color(0xaac6e8));
    d.rounded_rect({101,500,1100,19},9,color(0x193550));
    d.rounded_rect({101,500,1100*progress,19},9,color(0x27d9be));
    tx(d,font,atlas,time,102,554,23,color(0xd8e9ff));
    panel(d,1273,190,582,400);
    tx(d,font,atlas,"CONFIGURATION",1305,263,28,color(0x8fc8fb));
    tx(d,font,atlas,"Algorithm",1305,331,21,color(0x90a7c9));
    tx(d,font,atlas,"RandomX v1.2.1",1808,331,23,color(0xf1f8ff),Align::right);
    tx(d,font,atlas,"Mode",1305,392,21,color(0x90a7c9));
    tx(d,font,atlas,"LIGHT / INTERPRETED",1808,392,22,color(0xf1f8ff),Align::right);
    tx(d,font,atlas,"Run duration",1305,453,21,color(0x90a7c9));
    tx(d,font,atlas,std::to_string(durations[duration_index])+" seconds",1808,453,26,color(0x4af0bf),Align::right);
    tx(d,font,atlas,"Cache RAM",1305,521,21,color(0x90a7c9));
    tx(d,font,atlas,"about 256 MiB",1808,521,22,color(0xf1f8ff),Align::right);
    panel(d,60,615,1795,345);
    tx(d,font,atlas,"RESULTS & LOGGING",96,674,28,color(0x8fc8fb));
    tx(d,font,atlas,"Hashes computed",97,744,22,color(0x94acd0));
    tx(d,font,atlas,total,686,744,26, color(0xf1f8ff),Align::right);
    tx(d,font,atlas,"Last digest",96,807,22,color(0x94acd0));
    tx(d,font,atlas,b.last_hash[0]?std::string(b.last_hash):"Not available until the run ends",680,853,20,color(0xcbe1ff));
    const std::string message=(state==Failed?std::string(b.error):std::string(b.report_status));
    tx(d,font,atlas,message,100,926,19,color(state==Failed?0xff6f7e:0x80dbb7));
    tx(d,font,atlas,"BENCHMARK ONLY. NO COIN, NETWORK OR REWARD.",1278,744,21,color(0xffd283));
    tx(d,font,atlas,"Next: test fast mode, threads, JIT and thermals.",1278,799,18,color(0xb6cde8));
    tx(d,font,atlas,export_hint?"Report is automatically written after each run.":"/data/ps5-randomx/benchmark.log",1278,878,18,color(0x96b4d6));
    d.rounded_rect({0,1007,1920,73},0,color(0x07162b));
    tx(d,font,atlas,"X START  |  LEFT/RIGHT DURATION  |  O STOP / EXIT  |  TRIANGLE RESET  |  SQUARE LOG PATH",57,1054,20);
    renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
    hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
  }
  pad.close();renderer.release();display.close();hui::sys::quit();
}
