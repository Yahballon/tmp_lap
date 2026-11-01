// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 RANDOMX BENCHMARK by YAHBALLON v0.4.3
// Real RandomX v1.2.1, optional full-memory mode, interpreter; NOT a miner.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#include "randomx.h"
#include "auto_suite.hpp"
#include "jit_probe.hpp"
#include "jit_trace.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/mman.h>
#include <string>
#include <pthread.h>
#include <sys/stat.h>
using namespace hui::gfx;

// Hook provided for the instrumented pinned upstream RandomX archive.
// Keep the JIT diagnostics disabled for the stable AUTO/interpreter modes.
static std::atomic<int> g_rx_jit_trace_profile{-1};
static std::atomic<uint64_t> g_rx_jit_trace_session{0};
extern "C" void rx_ps5_jit_pointer(const char* stage, unsigned long long address) {
  const int p=g_rx_jit_trace_profile.load();
  if(p!=6 && p!=7)return;
  rxbench::jit_trace_pointer(stage,p,g_rx_jit_trace_session.load(),address);
}
extern "C" void rx_ps5_jit_stage(const char* stage, unsigned long long value) {
  const int p=g_rx_jit_trace_profile.load();
  if(p!=6 && p!=7)return;
  const int clipped=value>0x7fffffffULL ? 0x7fffffff : int(value);
  rxbench::jit_trace(stage,p,g_rx_jit_trace_session.load(),clipped);
}

namespace {
constexpr const char* kDataDir = "/data/ps5-randomx";
constexpr const char* kReport = "/data/ps5-randomx/benchmark.log";
constexpr const char* kKey = "PS5Coin benchmark - genesis-independent key - v0.1";
enum Stage : int { Ready=0, Preparing=1, Hashing=2, Completed=3, Stopped=4, Failed=5 };
struct Profile {
  const char* label;
  int threads;
  bool request_aes;
  bool full_mem;
  bool jit=false;
  bool probe_only=false;
  bool jit_init_only=false;
  bool buffer_probe_only=false;
};
constexpr Profile kProfiles[]={
  {"1 THREAD / SOFTWARE AES",1,false,false},
  {"1 THREAD / HARDWARE AES",1,true,false},
  {"2 THREADS / HARDWARE AES",2,true,false},
  {"4 THREADS / HARDWARE AES",4,true,false},
  {"1 THREAD / FULL MEMORY",1,true,true}, // Opt-in: >2 GiB RAM, no JIT
  {"JIT MEMORY PERMISSIONS",0,false,false,false,true}, // No code executed
  {"JIT VM INIT ONLY",0,true,false,false,false,true}, // No JIT hashes
  {"1 THREAD / SECURE JIT",1,true,false,true,false}, // Opt-in; may crash
  {"JIT 78 KiB BUFFER PROBE",0,false,false,false,false,false,true}
};
constexpr int kProfileCount=sizeof(kProfiles)/sizeof(kProfiles[0]);
struct Bench;
struct Worker {
  Bench* bench=nullptr;
  randomx_cache* cache=nullptr;
  randomx_dataset* dataset=nullptr;
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
  std::atomic<uint64_t> dataset_ready{0}, dataset_total{0}, prep_us{0};
  int seconds=30, profile_id=0, thread_count=1;
  bool hardware_aes=false, full_mem=false, jit_requested=false, probe_only=false, jit_init_only=false, buffer_probe_only=false;
  uint64_t session=0;
  rxbench::JitProbe jit_probe{};
  std::atomic<bool> jit_vector_ok{false};
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
void jitTrace(Bench* b,const char* stage,int value=0) {
  const Profile& p=kProfiles[b->profile_id];
  if(p.jit||p.probe_only||p.jit_init_only||p.buffer_probe_only)
    rxbench::jit_trace(stage,b->profile_id,b->session,value);
}
void report(Bench* b,int outcome) {
  mkdir(kDataDir,0777);
  FILE* f=std::fopen(kReport,"a");
  if(!f){std::snprintf(b->report_status,sizeof(b->report_status),"Cannot open %s",kReport);return;}
  const double elapsed=double(b->elapsed_us.load())/1000000.0;
  const uint64_t total=b->hashes.load();
  const double hps=elapsed>0?double(total)/elapsed:0.0;
  std::fprintf(f,"PS5 RANDOMX BENCHMARK by YAHBALLON v0.4.3\n");
  std::fprintf(f,"algorithm=RandomX-v1.2.1 mode=%s aes=%s jit=%s threads=%d profile=%d\n",
               b->probe_only?"jit-wx-probe":b->buffer_probe_only?"jit-buffer-write-probe":b->jit_init_only?"jit-vm-init-only":b->jit_requested?"light-secure-jit":
               b->full_mem?"full-memory-interpreter":"light-interpreter",
               b->hardware_aes?"hardware":"software",
               b->jit_requested?"secure-wx":b->jit_init_only?"init-only":"off",b->thread_count,b->profile_id);
  if(b->probe_only||b->jit_requested||b->jit_init_only||b->buffer_probe_only)
    std::fprintf(f,"jit_probe_mapped=%d jit_probe_rx=%d jit_probe_rw_restored=%d jit_probe_unmapped=%d jit_probe_errno=%d jit_probe_failed_at=%s jit_vector_ok=%d\n",
                 b->jit_probe.mapped?1:0,b->jit_probe.rx?1:0,b->jit_probe.rw_restored?1:0,
                 b->jit_probe.unmapped?1:0,b->jit_probe.error,b->jit_probe.failed_at,
                 b->jit_vector_ok.load()?1:0);
  std::fprintf(f,"dataset_bytes=%llu dataset_init_seconds=%.3f\n",
               (unsigned long long)(b->full_mem?
                 b->dataset_total.load()*uint64_t(RANDOMX_DATASET_ITEM_SIZE):0ULL),
               double(b->prep_us.load())/1000000.0);
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
  jitTrace(b,"before_final_report",outcome);
  report(b,outcome);
  jitTrace(b,"after_final_report",outcome);
  // Publish terminal state only after the report and all non-atomic fields are written.
  b->stage.store(outcome);
}
void* runWorker(void* context) {
  Worker* w=static_cast<Worker*>(context);
  Bench* b=w->bench;
  randomx_flags flags=b->hardware_aes?RANDOMX_FLAG_HARD_AES:RANDOMX_FLAG_DEFAULT;
  if(b->full_mem)flags|=RANDOMX_FLAG_FULL_MEM;
  if(b->jit_requested)flags|=(RANDOMX_FLAG_JIT|RANDOMX_FLAG_SECURE);
  jitTrace(b,"worker_before_vm_create",w->id);
  randomx_vm* vm=randomx_create_vm(flags,b->full_mem?nullptr:w->cache,
                                 b->full_mem?w->dataset:nullptr);
  jitTrace(b,"worker_after_vm_create",vm?1:0);
  // Before the benchmark clock starts, verify JIT output against interpreter.
  // This is an opt-in execution test only; failure never silently falls back.
  bool vector_ok=(vm!=nullptr);
  if(vm&&b->jit_requested){
    jitTrace(b,"before_reference_vm_create");
    randomx_vm* reference=randomx_create_vm(RANDOMX_FLAG_HARD_AES,w->cache,nullptr);
    jitTrace(b,"after_reference_vm_create",reference?1:0);
    if(!reference)vector_ok=false;
    else {
      std::array<uint8_t,64> sample{};
      std::memcpy(sample.data(),"PS5RandomX-v0.1 PS5Coin independent bench",40);
      sample[48]=static_cast<uint8_t>(w->id);
      uint8_t expected[RANDOMX_HASH_SIZE]{},actual[RANDOMX_HASH_SIZE]{};
      jitTrace(b,"before_reference_hash");
      randomx_calculate_hash(reference,sample.data(),sample.size(),expected);
      jitTrace(b,"after_reference_hash");
      jitTrace(b,"before_first_jit_hash");
      randomx_calculate_hash(vm,sample.data(),sample.size(),actual);
      jitTrace(b,"after_first_jit_hash");
      vector_ok=std::memcmp(expected,actual,RANDOMX_HASH_SIZE)==0;
      jitTrace(b,"before_reference_vm_destroy");
      randomx_destroy_vm(reference);
      jitTrace(b,"after_reference_vm_destroy");
    }
    b->jit_vector_ok.store(vector_ok);
    if(!vector_ok)std::snprintf(b->error,sizeof(b->error),
                                "JIT digest differs from interpreter or VM unavailable");
  }
  w->initialized=vector_ok;
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
      if(nonce==0)jitTrace(b,"before_first_timed_hash");
      randomx_calculate_hash(vm,input.data(),input.size(),hash.data());
      if(nonce==0)jitTrace(b,"after_first_timed_hash");
      ++nonce;
      b->hashes.fetch_add(1);
    }
    w->hashes=nonce;
    if(nonce)for(size_t i=0;i<hash.size();++i)
      std::snprintf(w->digest+2*i,3,"%02x",unsigned(hash[i]));
  }
  jitTrace(b,"before_worker_vm_destroy");
  randomx_destroy_vm(vm);
  jitTrace(b,"after_worker_vm_destroy");
  return nullptr;
}
void* runBench(void* context) {
  Bench* b=static_cast<Bench*>(context);
  jitTrace(b,"supervisor_enter");
  b->stage.store(Preparing);
  const randomx_flags available=randomx_get_flags();
  if(kProfiles[b->profile_id].request_aes &&
     !(available&RANDOMX_FLAG_HARD_AES)) {
    std::snprintf(b->error,sizeof(b->error),
                  "CPU/RandomX reports no hardware AES; select 1T SOFTWARE");
    finish(b,Failed);return nullptr;
  }
  b->hardware_aes=kProfiles[b->profile_id].request_aes;
  b->full_mem=kProfiles[b->profile_id].full_mem;
  b->jit_requested=kProfiles[b->profile_id].jit;
  b->probe_only=kProfiles[b->profile_id].probe_only;
  b->jit_init_only=kProfiles[b->profile_id].jit_init_only;
  b->buffer_probe_only=kProfiles[b->profile_id].buffer_probe_only;
  b->thread_count=kProfiles[b->profile_id].threads;
  // Probe only on explicit profile selection, not during normal AUTO mode.
  // Never executes the temporary page. May fail if PS5 HEN disallows RX rights.
  if(b->jit_requested||b->probe_only||b->jit_init_only||b->buffer_probe_only){
    jitTrace(b,"before_wx_probe");
    std::snprintf(b->report_status,sizeof(b->report_status),"Checking mmap RW -> RX -> RW...");
    b->jit_probe=rxbench::probe_wx_transition();
    jitTrace(b,"after_wx_probe",b->jit_probe.error);
    if(!b->jit_probe.passed()){
      std::snprintf(b->error,sizeof(b->error),
                    "JIT W^X denied at %s (errno %d)",
                    b->jit_probe.failed_at,b->jit_probe.error);
      finish(b,Failed);return nullptr;
    }
    if(b->probe_only){
      std::snprintf(b->report_status,sizeof(b->report_status),"W^X map protection probe passed");
      finish(b,Completed);return nullptr;
    }
    if(b->buffer_probe_only){
      // Isolate the real 77,824 byte mmap size from static assembler sourcing.
      // No JIT VM is created, no generated code executes.
      constexpr size_t kSize=77824;
      jitTrace(b,"buffer_probe_before_mmap",int(kSize));
      auto* mem=static_cast<uint8_t*>(mmap(nullptr,kSize,PROT_READ|PROT_WRITE,
                                            MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
      const int saved_errno=errno;
      jitTrace(b,"buffer_probe_after_mmap",mem==MAP_FAILED?0:1);
      if(mem==MAP_FAILED){
        std::snprintf(b->error,sizeof(b->error),"78KiB mmap failed, errno=%d",saved_errno);
        finish(b,Failed);return nullptr;
      }
      jitTrace(b,"buffer_probe_before_write");
      // Initial and final page checks. Log before each risky access.
      mem[0]=0x52;
      jitTrace(b,"buffer_probe_first_byte_written",int(mem[0]));
      mem[kSize-1]=0x58;
      jitTrace(b,"buffer_probe_last_byte_written",int(mem[kSize-1]));
      std::array<uint8_t,256> known{};
      for(size_t i=0;i<known.size();++i)known[i]=uint8_t(i);
      jitTrace(b,"buffer_probe_before_memcpy");
      std::memcpy(mem,known.data(),known.size());
      jitTrace(b,"buffer_probe_after_memcpy");
      const bool valid=std::memcmp(mem,known.data(),known.size())==0;
      jitTrace(b,"buffer_probe_verified",valid?1:0);
      munmap(mem,kSize);
      if(!valid){
        std::snprintf(b->error,sizeof(b->error),"78KiB mapping copy verification failed");
        finish(b,Failed);return nullptr;
      }
      finish(b,Completed);return nullptr;
    }
    jitTrace(b,"before_jit_flag_check",int(available));
    if(!(available&RANDOMX_FLAG_JIT)){
      std::snprintf(b->error,sizeof(b->error),"RandomX library reports JIT not supported");
      finish(b,Failed);return nullptr;
    }
  }
  // The large RandomX cache is initialized once, then shared read-only.
  jitTrace(b,"before_cache_allocate");
  randomx_cache* cache=randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
  jitTrace(b,"after_cache_allocate",cache?1:0);
  if(!cache) {
    std::snprintf(b->error,sizeof(b->error),"RandomX cache allocation failed (~256 MiB)");
    finish(b,Failed);return nullptr;
  }
  jitTrace(b,"before_cache_init");
  randomx_init_cache(cache,kKey,std::strlen(kKey));
  jitTrace(b,"after_cache_init");
  if(b->stop.load()) {
    randomx_release_cache(cache);
    finish(b,Stopped);return nullptr;
  }
  if(b->jit_init_only) {
    // Deliberately no generated-code execution or hash calculation.
    jitTrace(b,"before_init_only_vm_create");
    randomx_vm* vm=randomx_create_vm(RANDOMX_FLAG_JIT|RANDOMX_FLAG_SECURE|
                                     RANDOMX_FLAG_HARD_AES,cache,nullptr);
    jitTrace(b,"after_init_only_vm_create",vm?1:0);
    if(!vm){
      std::snprintf(b->error,sizeof(b->error),"RandomX JIT VM allocation rejected");
      randomx_release_cache(cache);
      finish(b,Failed);return nullptr;
    }
    jitTrace(b,"before_init_only_vm_destroy");
    randomx_destroy_vm(vm);
    jitTrace(b,"after_init_only_vm_destroy");
    randomx_release_cache(cache);
    finish(b,Completed);return nullptr;
  }
  randomx_dataset* dataset=nullptr;
  if(b->full_mem) {
    // Allocates ~2,080 MiB. The console can refuse this; never auto-start it.
    // Keep the cache available throughout initialization then share the
    // immutable finished dataset with all VMs.
    std::snprintf(b->report_status,sizeof(b->report_status),
                  "Allocating RandomX full dataset (~2.08 GiB)...");
    dataset=randomx_alloc_dataset(RANDOMX_FLAG_DEFAULT);
    if(!dataset){
      std::snprintf(b->error,sizeof(b->error),
                    "Not enough memory for full dataset (~2.08 GiB)");
      randomx_release_cache(cache);
      finish(b,Failed);return nullptr;
    }
    const uint64_t t0=uint64_t(hui::sys::monotonic_us());
    const unsigned long count=randomx_dataset_item_count();
    b->dataset_total.store(count);
    constexpr unsigned long kChunk=32768UL;
    for(unsigned long offset=0;offset<count&&!b->stop.load();) {
      const unsigned long n=std::min(kChunk,count-offset);
      randomx_init_dataset(dataset,cache,offset,n);
      offset+=n;
      b->dataset_ready.store(offset);
    }
    b->prep_us.store(uint64_t(hui::sys::monotonic_us())-t0);
    if(b->stop.load()){
      randomx_release_dataset(dataset);
      randomx_release_cache(cache);
      finish(b,Stopped);return nullptr;
    }
  }
  int launched=0;
  bool failed=false;
  for(int i=0;i<b->thread_count;i++) {
    Worker& w=b->workers[i];
    w.bench=b;w.cache=cache;w.dataset=dataset;w.id=i;w.hashes=0;
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
  if(dataset)randomx_release_dataset(dataset);
  randomx_release_cache(cache);
  finish(b,failed?Failed:b->stop.load()?Stopped:Completed);
  return nullptr;
}
bool start(Bench& b,int seconds,int profile_id){
  if(b.join_needed)return false;
  b.seconds=seconds;b.profile_id=profile_id;
  b.thread_count=kProfiles[profile_id].threads;
  b.hardware_aes=false;b.jit_requested=false;b.probe_only=false;b.jit_init_only=false;b.buffer_probe_only=false;
  b.session=uint64_t(hui::sys::monotonic_us());
  g_rx_jit_trace_profile.store((profile_id==6||profile_id==7)?profile_id:-1);
  g_rx_jit_trace_session.store(b.session);
  b.jit_probe=rxbench::JitProbe{};b.jit_vector_ok.store(false);
  b.hashes.store(0);b.elapsed_us.store(0);b.start_us.store(0);
  b.ready.store(0);b.go.store(false);b.stop.store(false);
  b.dataset_ready.store(0);b.dataset_total.store(0);b.prep_us.store(0);
  b.error[0]=0;b.last_hash[0]=0;
  for(auto& w:b.workers){w.hashes=0;w.digest[0]=0;}
  std::snprintf(b.report_status,sizeof(b.report_status),"Initializing RandomX cache...");
  jitTrace(&b,"ui_start_selected_mode");
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
void writeAutoSummary(const rxbench::AutoSuite& suite) {
  mkdir(kDataDir,0777);
  FILE* f=std::fopen(kReport,"a");
  if(!f)return;
  std::fprintf(f,"PS5 RANDOMX AUTO TEST by YAHBALLON v0.4.3\n");
  std::fprintf(f,"auto_total=%d completed_tests=%d duration_each=%d include_full=%d\n",
               suite.total,suite.recorded,suite.duration,suite.with_full?1:0);
  for(int i=0;i<suite.recorded;i++){
    const rxbench::AutoResult& r=suite.results[i];
    std::fprintf(f,"test_%d profile=%d name=%s elapsed_seconds=%.3f hashes=%llu hashrate_hps=%.4f outcome=%d\n",
                 i+1,r.profile,kProfiles[r.profile].label,double(r.elapsed_us)/1000000.0,
                 (unsigned long long)r.hashes,r.hps,r.outcome);
  }
  std::fprintf(f,"auto_status=%s\n\n",suite.cancelled?"interrupted":
               suite.recorded==suite.total&&suite.results[suite.recorded-1].outcome==Completed?
               "completed":"failed");
  std::fclose(f);
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
  Bench b;rxbench::AutoSuite suite;
  // 0 = AUTO LIGHT (default); 1 = AUTO ALL (opt-in >2 GiB RAM);
  // 2+ = manual profiles; all JIT modes remain opt-in.
  int duration_index=1,mode_index=0;
  constexpr int durations[]={10,30,60};
  constexpr int kModeCount=kProfileCount+2;
  unsigned prior=0;bool exit_requested=false;bool export_hint=false;
  for(;;){
    reap(b);
    // Complete the current run, save its measurements and immediately begin
    // the next profile. This runs on the UI thread, after worker join.
    const int finished_state=b.stage.load();
    if(suite.active&&!b.join_needed&&
       (finished_state==Completed||finished_state==Stopped||finished_state==Failed)){
      const bool next=suite.record(finished_state,finished_state==Completed,
                                  b.hashes.load(),b.elapsed_us.load());
      if(next){
        if(!start(b,suite.duration,suite.step)) {
          suite.cancel();
          suite.active=false;suite.done=true;
          writeAutoSummary(suite);
        }
      }else {
        writeAutoSummary(suite);
      }
    }
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
    if(!busy && !suite.active){
      if(key(hui::pad_bits::kLeft))duration_index=(duration_index+2)%3;
      if(key(hui::pad_bits::kRight))duration_index=(duration_index+1)%3;
      if(key(hui::pad_bits::kUp)||key(hui::pad_bits::kL1))mode_index=(mode_index+kModeCount-1)%kModeCount;
      if(key(hui::pad_bits::kDown)||key(hui::pad_bits::kR1))mode_index=(mode_index+1)%kModeCount;
      if(key(hui::pad_bits::kCross)){
        export_hint=false;
        if(mode_index<2){
          suite.begin(mode_index==1,durations[duration_index]);
          if(!start(b,suite.duration,0)){
            suite.cancel();suite.active=false;suite.done=true;
            writeAutoSummary(suite);
          }
        }else {
          suite.done=false;
          start(b,durations[duration_index],mode_index-2);
        }
      }
      if(key(hui::pad_bits::kTriangle)){
        suite.done=false;suite.recorded=0;
        b.hashes.store(0);b.elapsed_us.store(0);b.last_hash[0]=0;
        b.stage.store(Ready);export_hint=false;
      }
      if(key(hui::pad_bits::kCircle))exit_requested=true;
      if(key(hui::pad_bits::kSquare))export_hint=true;
    }else if(key(hui::pad_bits::kCircle)||key(hui::pad_bits::kOptions)){
      suite.cancel();
      b.stop.store(true);
    }
    if(exit_requested && !b.join_needed)break;

    const uint64_t hashes=b.hashes.load();
    const uint64_t elapsed=b.stage.load()==Hashing?
       uint64_t(hui::sys::monotonic_us())-b.start_us.load():b.elapsed_us.load();
    const double hps=elapsed>0?double(hashes)*1000000.0/double(elapsed):0.0;
    const double seconds=double(elapsed)/1000000.0;
    const bool preparing_dataset=state==Preparing&&b.dataset_total.load()>0;
    const float progress=preparing_dataset?
      std::clamp(float(double(b.dataset_ready.load())/double(b.dataset_total.load())),0.f,1.f):
      std::clamp(float(seconds/double(b.seconds)),0.f,1.f);
    const bool chosen_auto=mode_index<2;
    const int shown_profile=(busy||suite.active)?b.profile_id:
                             (chosen_auto?0:mode_index-2);
    const Profile& selected=kProfiles[shown_profile];
    const char* auto_title=(mode_index==1?"AUTO ALL 5 (FULL RAM)":"AUTO 4 LIGHT MODES");
    const std::string shown_mode=suite.active?
      ("AUTO STEP "+std::to_string(suite.step+1)+"/"+std::to_string(suite.total)+": "+selected.label):
      (chosen_auto?std::string(auto_title):std::string(selected.label));
    char rate[80],time[80],total[80];
    if((selected.probe_only||selected.buffer_probe_only)&&state==Completed)
      std::snprintf(rate,sizeof(rate),"W^X PASS");
    else if(selected.jit_init_only&&state==Completed)
      std::snprintf(rate,sizeof(rate),"VM INIT PASS");
    else std::snprintf(rate,sizeof(rate),"%.2f H/s",hps);
    std::snprintf(time,sizeof(time),"%.1f / %d sec",seconds,b.seconds);
    std::snprintf(total,sizeof(total),"%llu",(unsigned long long)hashes);
    DrawList d;
    d.gradient_rect({0,0,1920,1080},0,color(0x040915),color(0x0a1d35));
    d.circle(1770,250,290,color(0x0c3553,.20f));
    d.rounded_rect({0,0,1920,102},0,color(0x061225));
    d.rounded_rect({39,24,52,52},13,color(0x0761cc));
    tx(d,font,atlas,"RX",65,60,27,color(0xffffff),Align::center);
    tx(d,font,atlas,"PS5 RANDOMX BENCHMARK  by YAHBALLON",113,61,33);
    tx(d,font,atlas,"v0.4.3",1879,60,24,color(0xc9e4ff),Align::right);
    tx(d,font,atlas,"REAL RANDOMX PROOF-OF-WORK PERFORMANCE TEST",63,165,19,color(0x70bbfd));
    panel(d,60,190,1190,400);
    tx(d,font,atlas,preparing_dataset?"INITIALIZING FULL DATASET":
       suite.active?"AUTO BENCHMARK RUNNING":
       suite.done?(suite.cancelled?"AUTO TEST CANCELLED":suite.recorded==suite.total&&suite.results[suite.recorded-1].outcome==Completed?"AUTO TEST COMPLETE":"AUTO TEST FAILED"):statusName(state),
       97,257,30,color(busy?0x57dcff:state==Failed?0xff6488:0x69efbd));
    tx(d,font,atlas,rate,100,392,88);
    tx(d,font,atlas,shown_mode+(selected.jit?" / SECURE JIT":selected.probe_only||selected.buffer_probe_only||selected.jit_init_only?" / NO CODE EXECUTION":" / NO JIT"),103,455,23,color(0xaac6e8));
    d.rounded_rect({101,500,1100,19},9,color(0x193550));
    d.rounded_rect({101,500,1100*progress,19},9,color(0x27d9be));
    tx(d,font,atlas,preparing_dataset?("Dataset prepared: "+std::to_string(int(progress*100.f))+"%"):std::string(time),
       102,554,23,color(0xd8e9ff));
    panel(d,1273,190,582,400);
    tx(d,font,atlas,"CONFIGURATION",1305,263,28,color(0x8fc8fb));
    tx(d,font,atlas,"Algorithm",1305,331,21,color(0x90a7c9));
    tx(d,font,atlas,"RandomX v1.2.1",1808,331,23,color(0xf1f8ff),Align::right);
    tx(d,font,atlas,"Mode",1305,385,21,color(0x90a7c9));
    tx(d,font,atlas,!busy&&chosen_auto&&!suite.active?"AUTO SEQUENTIAL":
       selected.probe_only?"MEMORY W^X PROBE":selected.buffer_probe_only?"78 KiB WRITE PROBE":selected.jit_init_only?"JIT VM INIT ONLY":selected.jit?"LIGHT / SECURE JIT":
       selected.full_mem?"FULL / INTERPRETED":"LIGHT / INTERPRETED",
       1808,385,22,color(0xf1f8ff),Align::right);
    tx(d,font,atlas,"CPU workers",1305,433,21,color(0x90a7c9));
    tx(d,font,atlas,!busy&&chosen_auto&&!suite.active?"1 / 1 / 2 / 4":
       selected.probe_only||selected.buffer_probe_only||selected.jit_init_only?"NOT REQUIRED":
       std::to_string(selected.threads)+" / "+(selected.request_aes?"HW AES":"SOFT AES"),
       1808,433,22,color(0xf1f8ff),Align::right);
    tx(d,font,atlas,"Run duration",1305,478,21,color(0x90a7c9));
    tx(d,font,atlas,std::to_string(durations[duration_index])+" seconds",1808,478,26,color(0x4af0bf),Align::right);
    tx(d,font,atlas,selected.full_mem?"Dataset RAM":"Cache RAM",1305,531,21,color(0x90a7c9));
    tx(d,font,atlas,(suite.active&&suite.with_full)||(!suite.active&&mode_index==1)?
       "UP TO 2.08 GiB":"about 256 MiB",1808,531,22,color(0xf1f8ff),Align::right);
    panel(d,60,615,1795,345);
    tx(d,font,atlas,suite.active||suite.done?"AUTO TEST RESULTS":"RESULTS & LOGGING",
       96,674,28,color(0x8fc8fb));
    if(suite.active||suite.done){
      for(int i=0;i<suite.recorded;i++){
        const rxbench::AutoResult& r=suite.results[i];
        char hps_label[56];
        std::snprintf(hps_label,sizeof(hps_label),"%.2f H/s",r.hps);
        const float y=722.f+42.f*float(i);
        tx(d,font,atlas,kProfiles[r.profile].label,100,y,20,color(0xb6d5f6));
        tx(d,font,atlas,hps_label,980,y,24,
           color(r.outcome==Completed?0x68edc1:0xffc370),Align::right);
      }
      if(suite.active)
        tx(d,font,atlas,"Current: "+std::to_string(suite.step+1)+"/"+
           std::to_string(suite.total)+"  |  "+std::to_string(hashes)+" hashes",103,926,
           20,color(0x8fd7fd));
      else tx(d,font,atlas,suite.cancelled?"AUTO TEST CANCELLED - See benchmark.log":
        suite.recorded==suite.total&&suite.results[suite.recorded-1].outcome==Completed?
        "AUTO TEST COMPLETE - See benchmark.log":"AUTO TEST FAILED - See benchmark.log",103,926,
                   20,color(0x68edc1));
    }else{
      tx(d,font,atlas,"Hashes computed",97,744,22,color(0x94acd0));
      tx(d,font,atlas,total,686,744,26,color(0xf1f8ff),Align::right);
      tx(d,font,atlas,"Last digest",96,807,22,color(0x94acd0));
      tx(d,font,atlas,b.last_hash[0]?std::string(b.last_hash):
         "Not available until the run ends",680,853,20,color(0xcbe1ff));
      const std::string message=(state==Failed?std::string(b.error):std::string(b.report_status));
      tx(d,font,atlas,message,100,926,19,color(state==Failed?0xff6f7e:0x80dbb7));
    }
    tx(d,font,atlas,"BENCHMARK ONLY. NO COIN, NETWORK OR REWARD.",1278,744,21,color(0xffd283));
    tx(d,font,atlas,(mode_index==1||selected.full_mem)?
       "FULL MEMORY IS OPT-IN: >2 GiB RAM REQUIRED.":
       selected.jit?"JIT EXPERIMENTAL: W^X PREFLIGHT REQUIRED; MAY CRASH.":
       selected.probe_only?"W^X PROBE DOES NOT EXECUTE DYNAMIC CODE.":
       selected.buffer_probe_only?"BUFFER PROBE WRITES KNOWN DATA ONLY; NO JIT.":
       selected.jit_init_only?"VM INIT DOES NOT EXECUTE GENERATED CODE.":
       "AUTO LIGHT RUNS FOUR PROFILES. NO MANUAL SWITCHING.",
       1278,799,18,color(0xb6cde8));
    tx(d,font,atlas,export_hint?"Every run writes a report. JIT stages write jit_trace.log.":
       (selected.jit||selected.jit_init_only||selected.probe_only)?
       "/data/ps5-randomx/jit_trace.log":"/data/ps5-randomx/benchmark.log",1278,878,18,color(0x96b4d6));
    d.rounded_rect({0,1007,1920,73},0,color(0x07162b));
    tx(d,font,atlas,"X AUTO START   UP/DOWN CHOOSE AUTO OR SINGLE   LEFT/RIGHT TIME   O STOP/EXIT   TRIANGLE RESET",57,1054,20);
    renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
    hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
  }
  pad.close();renderer.release();display.close();hui::sys::quit();
}
