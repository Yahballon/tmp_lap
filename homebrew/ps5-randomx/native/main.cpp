// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 RANDOMX BENCHMARK by YAHBALLON v0.2.0
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
struct Profile {
  const char* label;
  int threads;
  bool request_aes;
};
constexpr Profile kProfiles[]={
  {"1 THREAD / SOFTWARE AES",1,false},
  {"1 THREAD / HARDWARE AES",1,true},
  {"2 THREADS / HARDWARE AES",2,true},
  {"4 THREADS / HARDWARE AES",4,true}
};
struct Bench;
struct Worker {
  Bench* bench=nullptr;
  randomx_cache* cache=nullptr;
  int id=0;
  bool initialized=false;
  uint64_t hashes=0;
  char digest[65]="";
};
struct Bench {
  std::atomic<int> stage{Ready};
  std::atomic<bool> stop{false}, go{false};
  std::atomic<int> ready{0};
  std::atomic<uint64_t> hashes{0}, elapsed_us{0}, start_us{0};
  int seconds=30, profile_id=0, thread_count=1;
  bool hardware_aes=false;
  pthread_t thread{}, worker_threads[4]{};
  Worker workers[4]{};
  bool join_needed=false;
  char error[160]="";
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
void report(Bench* b,int outcome) {
  mkdir(kDataDir,0777);
  FILE* f=std::fopen(kReport,"a");
  if(!f){std::snprintf(b->report_status,sizeof(b->report_status),"Cannot open %s",kReport);return;}
  const double elapsed=double(b->elapsed_us.load())/1000000.0;
  const uint64_t total=b->hashes.load();
  const double hps=elapsed>0?double(total)/elapsed:0.0;
  std::fprintf(f,"PS5 RANDOMX BENCHMARK by YAHBALLON v0.2.0\n");
  std::fprintf(f,"algorithm=RandomX-v1.2.1 mode=light-interpreter aes=%s jit=off threads=%d profile=%d\n",
               b->hardware_aes?"hardware":"software",b->thread_count,b->profile_id);
  std::fprintf(f,"requested_seconds=%d elapsed_seconds=%.3f hashes=%llu hashrate_hps=%.4f\n",
               b->seconds,elapsed,(unsigned long long)total,hps);
  for(int i=0;i<b->thread_count;i++)
    std::fprintf(f,"thread_%d_hashes=%llu\n",i,
                 (unsigned long long)b->workers[i].hashes);
  std::fprintf(f,"last_hash=%s\nstatus=%s\n",b->last_hash,
               outcome==Completed?"completed":outcome==Stopped?"interrupted":"failed");
  if(outcome==Failed)std::fprintf(f,"error=%s\n",b->error);
  std::fprintf(f,"\n");
  std::fclose(f);
  std::snprintf(b->report_status,sizeof(b->report_status),"Saved to %s",kReport);
}
void finish(Bench* b,int outcome) {
  report(b,outcome);
  // Publish terminal state only after the report and all non-atomic fields are written.
  b->stage.store(outcome);
}
void* runWorker(void* context) {
  Worker* w=static_cast<Worker*>(context);
  Bench* b=w->bench;
  const randomx_flags flags=b->hardware_aes?RANDOMX_FLAG_HARD_AES:RANDOMX_FLAG_DEFAULT;
  randomx_vm* vm=randomx_create_vm(flags,w->cache,nullptr);
  w->initialized=(vm!=nullptr);
  b->ready.fetch_add(1);
  if(!vm)return nullptr;
  while(!b->go.load()&&!b->stop.load())hui::sys::sleep_us(1000);
  if(!b->stop.load()) {
    std::array<uint8_t,64> input{};
    std::memcpy(input.data(),"PS5RandomX-v0.1 PS5Coin independent bench",40);
    input[48]=static_cast<uint8_t>(w->id); // distinct input stream per worker
    std::array<uint8_t,RANDOMX_HASH_SIZE> hash{};
    const uint64_t started=b->start_us.load();
    const uint64_t deadline=started+uint64_t(b->seconds)*1000000ULL;
    uint64_t nonce=0;
    while(!b->stop.load()&&uint64_t(hui::sys::monotonic_us())<deadline) {
      std::memcpy(input.data()+56,&nonce,sizeof(nonce));
      randomx_calculate_hash(vm,input.data(),input.size(),hash.data());
      ++nonce;
      b->hashes.fetch_add(1);
    }
    w->hashes=nonce;
    if(nonce)for(size_t i=0;i<hash.size();++i)
      std::snprintf(w->digest+2*i,3,"%02x",unsigned(hash[i]));
  }
  randomx_destroy_vm(vm);
  return nullptr;
}
void* runBench(void* context) {
  Bench* b=static_cast<Bench*>(context);
  b->stage.store(Preparing);
  const randomx_flags available=randomx_get_flags();
  if(kProfiles[b->profile_id].request_aes &&
     !(available&RANDOMX_FLAG_HARD_AES)) {
    std::snprintf(b->error,sizeof(b->error),
                  "CPU/RandomX reports no hardware AES; select 1T SOFTWARE");
    finish(b,Failed);return nullptr;
  }
  b->hardware_aes=kProfiles[b->profile_id].request_aes;
  b->thread_count=kProfiles[b->profile_id].threads;
  // The large RandomX cache is initialized once, then shared read-only.
  randomx_cache* cache=randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
  if(!cache) {
    std::snprintf(b->error,sizeof(b->error),"RandomX cache allocation failed (~256 MiB)");
    finish(b,Failed);return nullptr;
  }
  randomx_init_cache(cache,kKey,std::strlen(kKey));
  if(b->stop.load()) {
    randomx_release_cache(cache);
    finish(b,Stopped);return nullptr;
  }
  int launched=0;
  bool failed=false;
  for(int i=0;i<b->thread_count;i++) {
    Worker& w=b->workers[i];
    w.bench=b;w.cache=cache;w.id=i;w.hashes=0;
    w.initialized=false;w.digest[0]=0;
    if(pthread_create(&b->worker_threads[i],nullptr,runWorker,&w)!=0) {
      std::snprintf(b->error,sizeof(b->error),
                    "Cannot start worker %d of %d",i+1,b->thread_count);
      failed=true;break;
    }
    ++launched;
  }
  while(!failed&&!b->stop.load()&&b->ready.load()<launched)
    hui::sys::sleep_us(1000);
  if(!failed&&!b->stop.load())for(int i=0;i<launched;i++)
    if(!b->workers[i].initialized) {
      failed=true;
      std::snprintf(b->error,sizeof(b->error),"RandomX VM allocation failed for thread %d",i);
      break;
    }
  if(!failed&&!b->stop.load()){
    b->start_us.store(uint64_t(hui::sys::monotonic_us()));
    b->stage.store(Hashing);
    b->go.store(true);
  } else {
    b->stop.store(true);
    b->go.store(true);
  }
  for(int i=0;i<launched;i++)pthread_join(b->worker_threads[i],nullptr);
  if(b->start_us.load()){
    b->elapsed_us.store(uint64_t(hui::sys::monotonic_us())-b->start_us.load());
  }
  if(b->workers[0].digest[0])
    std::memcpy(b->last_hash,b->workers[0].digest,sizeof(b->last_hash));
  randomx_release_cache(cache);
  finish(b,failed?Failed:b->stop.load()?Stopped:Completed);
  return nullptr;
}
bool start(Bench& b,int seconds,int profile_id){
  if(b.join_needed)return false;
  b.seconds=seconds;b.profile_id=profile_id;
  b.thread_count=kProfiles[profile_id].threads;
  b.hardware_aes=false;
  b.hashes.store(0);b.elapsed_us.store(0);b.start_us.store(0);
  b.ready.store(0);b.go.store(false);b.stop.store(false);
  b.error[0]=0;b.last_hash[0]=0;
  for(auto& w:b.workers){w.hashes=0;w.digest[0]=0;}
  std::snprintf(b.report_status,sizeof(b.report_status),"Initializing RandomX cache...");
  b.stage.store(Preparing);
  if(pthread_create(&b.thread,nullptr,runBench,&b)!=0){
    std::snprintf(b.error,sizeof(b.error),"Cannot create supervisor thread");
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
  Bench b;int duration_index=1,profile_index=1;constexpr int durations[]={10,30,60};
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
      if(samples[i].left_y<68)buttons|=hui::pad_bits::kUp;
      else if(samples[i].left_y>188)buttons|=hui::pad_bits::kDown;
    }
    const unsigned pressed=buttons&~prior;prior=buttons;
    const auto key=[&](unsigned bit){return (pressed&bit)!=0;};
    const int state=b.stage.load();
    const bool busy=state==Preparing||state==Hashing;
    if(!busy){
      if(key(hui::pad_bits::kLeft))duration_index=(duration_index+2)%3;
      if(key(hui::pad_bits::kRight))duration_index=(duration_index+1)%3;
      if(key(hui::pad_bits::kUp)||key(hui::pad_bits::kL1))profile_index=(profile_index+3)%4;
      if(key(hui::pad_bits::kDown)||key(hui::pad_bits::kR1))profile_index=(profile_index+1)%4;
      if(key(hui::pad_bits::kCross)){export_hint=false;start(b,durations[duration_index],profile_index);}
      if(key(hui::pad_bits::kTriangle)){b.hashes.store(0);b.elapsed_us.store(0);b.last_hash[0]=0;b.stage.store(Ready);export_hint=false;}
      if(key(hui::pad_bits::kCircle))exit_requested=true;
      if(key(hui::pad_bits::kSquare))export_hint=true;
    }else if(key(hui::pad_bits::kCircle)||key(hui::pad_bits::kOptions)){
      b.stop.store(true);
    }
    if(exit_requested && !b.join_needed)break;

    const uint64_t hashes=b.hashes.load();
    const uint64_t elapsed=b.stage.load()==Hashing?
       uint64_t(hui::sys::monotonic_us())-b.start_us.load():b.elapsed_us.load();
    const double hps=elapsed>0?double(hashes)*1000000.0/double(elapsed):0.0;
    const double seconds=double(elapsed)/1000000.0;
    const float progress=std::clamp(float(seconds/double(b.seconds)),0.f,1.f);
    const Profile& selected=kProfiles[busy?b.profile_id:profile_index];
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
    tx(d,font,atlas,"v0.2.0",1879,60,24,color(0xc9e4ff),Align::right);
    tx(d,font,atlas,"REAL RANDOMX PROOF-OF-WORK PERFORMANCE TEST",63,165,19,color(0x70bbfd));
    panel(d,60,190,1190,400);
    tx(d,font,atlas,statusName(state),97,257,30,color(busy?0x57dcff:state==Failed?0xff6488:0x69efbd));
    tx(d,font,atlas,rate,100,392,88);
    tx(d,font,atlas,std::string(selected.label)+"  /  NO JIT  /  LIGHT MODE",103,455,23,color(0xaac6e8));
    d.rounded_rect({101,500,1100,19},9,color(0x193550));
    d.rounded_rect({101,500,1100*progress,19},9,color(0x27d9be));
    tx(d,font,atlas,time,102,554,23,color(0xd8e9ff));
    panel(d,1273,190,582,400);
    tx(d,font,atlas,"CONFIGURATION",1305,263,28,color(0x8fc8fb));
    tx(d,font,atlas,"Algorithm",1305,331,21,color(0x90a7c9));
    tx(d,font,atlas,"RandomX v1.2.1",1808,331,23,color(0xf1f8ff),Align::right);
    tx(d,font,atlas,"Mode",1305,385,21,color(0x90a7c9));
    tx(d,font,atlas,"LIGHT / INTERPRETED",1808,385,22,color(0xf1f8ff),Align::right);
    tx(d,font,atlas,"CPU workers",1305,433,21,color(0x90a7c9));
    tx(d,font,atlas,std::to_string(selected.threads)+" / "+(selected.request_aes?"HW AES":"SOFT AES"),1808,433,22,color(0xf1f8ff),Align::right);
    tx(d,font,atlas,"Run duration",1305,478,21,color(0x90a7c9));
    tx(d,font,atlas,std::to_string(durations[duration_index])+" seconds",1808,478,26,color(0x4af0bf),Align::right);
    tx(d,font,atlas,"Cache RAM",1305,531,21,color(0x90a7c9));
    tx(d,font,atlas,"about 256 MiB",1808,531,22,color(0xf1f8ff),Align::right);
    panel(d,60,615,1795,345);
    tx(d,font,atlas,"RESULTS & LOGGING",96,674,28,color(0x8fc8fb));
    tx(d,font,atlas,"Hashes computed",97,744,22,color(0x94acd0));
    tx(d,font,atlas,total,686,744,26, color(0xf1f8ff),Align::right);
    tx(d,font,atlas,"Last digest",96,807,22,color(0x94acd0));
    tx(d,font,atlas,b.last_hash[0]?std::string(b.last_hash):"Not available until the run ends",680,853,20,color(0xcbe1ff));
    const std::string message=(state==Failed?std::string(b.error):std::string(b.report_status));
    tx(d,font,atlas,message,100,926,19,color(state==Failed?0xff6f7e:0x80dbb7));
    tx(d,font,atlas,"BENCHMARK ONLY. NO COIN, NETWORK OR REWARD.",1278,744,21,color(0xffd283));
    tx(d,font,atlas,"NEXT: Fast memory mode / JIT only after PS5 checks.",1278,799,18,color(0xb6cde8));
    tx(d,font,atlas,export_hint?"Report is automatically written after each run.":"/data/ps5-randomx/benchmark.log",1278,878,18,color(0x96b4d6));
    d.rounded_rect({0,1007,1920,73},0,color(0x07162b));
    tx(d,font,atlas,"X START   UP/DOWN or L1/R1 PROFILE   LEFT/RIGHT TIME   O STOP/EXIT   TRIANGLE RESET   SQUARE LOG",57,1054,20);
    renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
    hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
  }
  pad.close();renderer.release();display.close();hui::sys::quit();
}
