// SPDX-License-Identifier: GPL-3.0-or-later
// PPSA99847: real Godot 4.7.2 GLES3 RenderingServer canvas test.
// No native sprite shaders or glDrawArrays: Godot must draw the scene.
#include "graphics_hooks_ps5.h"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/system.hpp"
#include <EGL/egl.h>
#include <GL/glcorearb.h>
#include <cstdio>
#include <ctime>
#include <cstdint>
#include <algorithm>
#include <sys/stat.h>
#include <string>
// Explicit, validated user-selected Godot 4 pack only; bundled visual
// scene stays default. Never auto-load stale USB content.
#define GODOT_PS5_NATIVE_IO 1
#include "pck_launch_config.hpp"
extern "C" int godot_ps5_run_pack(const char *);

namespace {
hui::ps5::Display display;
bool opened=false;
unsigned frames=0;
// M13 opt-in profiler. Native PS5 timer around the real EGL/Godot swap:
// separates time spent updating/rendering a Godot frame (CPU-side) from
// time blocked in Display::swap() (vsync / GPU / driver / compositor).
// Experimental signed v0.1.7: used by TESTBUILD hardware CPU/EGL attribution.\n// Logs are deliberately bounded so profiling does not create its own
// log-I/O-induced hitch. No readback, glFinish or CPU/GPU sync introduced.
double ps5_now_ms(){
    struct timespec t{};
    return clock_gettime(CLOCK_MONOTONIC, &t)==0
        ? static_cast<double>(t.tv_sec)*1000.0+static_cast<double>(t.tv_nsec)*1.0e-6 : 0.0;
}
double perf_last_swap_complete_ms=0.0;
double perf_epoch_ms=0.0;
unsigned perf_samples=0,perf_window_spikes=0,perf_window_logged=0;
double perf_cpu_ms=0.0,perf_swap_ms=0.0,perf_max_cpu_ms=0.0,perf_max_swap_ms=0.0,perf_max_frame_ms=0.0;
void perf_report(double cpu, double swap, double begin, double end){
    if (perf_epoch_ms<=0.0)perf_epoch_ms=begin;
    const double frame=cpu+swap;
    ++perf_samples;
    perf_cpu_ms+=cpu;
    perf_swap_ms+=swap;
    perf_max_cpu_ms=std::max(perf_max_cpu_ms,cpu);
    perf_max_swap_ms=std::max(perf_max_swap_ms,swap);
    perf_max_frame_ms=std::max(perf_max_frame_ms,frame);
    if (frame>=80.0){
        ++perf_window_spikes;
        if(perf_window_logged<8){
            ++perf_window_logged;
            std::printf("GODOT_PS5_M13_FRAME_SPIKE frame=%u since_boot_ms=%.1f cpu_before_swap_ms=%.1f egl_swap_ms=%.1f total_ms=%.1f spike=%u\n",
                frames,end-perf_epoch_ms,cpu,swap,frame,perf_window_spikes);
        }
    }
    if(end-perf_epoch_ms>=5000.0){
        std::printf("GODOT_PS5_M13_FRAME_5S frames=%u avg_cpu_ms=%.2f avg_swap_ms=%.2f max_cpu_ms=%.1f max_swap_ms=%.1f max_frame_ms=%.1f spikes_80ms=%u\n",
            perf_samples, perf_cpu_ms/std::max(1u,perf_samples),
            perf_swap_ms/std::max(1u,perf_samples),
            perf_max_cpu_ms,perf_max_swap_ms,perf_max_frame_ms,perf_window_spikes);
        perf_epoch_ms=end;perf_samples=0;perf_window_spikes=0;perf_window_logged=0;
        perf_cpu_ms=perf_swap_ms=perf_max_cpu_ms=perf_max_swap_ms=perf_max_frame_ms=0.0;
    }
}
constexpr const char *logpath="/data/GodotPS5/gles3-runtime.log";
void record(const char *name,int value=-1){
    if(FILE*f=std::fopen(logpath,"ab")){
        if(value>=0)std::fprintf(f,"[GodotPS5 GLES3 0.1.1] %s=%d\n",name,value);
        else std::fprintf(f,"[GodotPS5 GLES3 0.1.1] %s\n",name);
        std::fclose(f);
    }
}
void record_pack(const char *source,const char *path){
    if(FILE*f=std::fopen(logpath,"ab")){
        std::fprintf(f,"[GodotPS5 GLES3 0.1.1] PACK_SOURCE=%s PACK=%s\n",source,path);
        std::fclose(f);
    }
}
bool open_context(int w,int h){
    if(opened)return false;
    if(!display.open(w,h)){
        record("EGL_OPEN_FAILED",int(display.last_error()));return false;
    }
    opened=true;record("EGL_OPEN_OK");
    record("WIDTH",display.width());record("HEIGHT",display.height());
    if(FILE*f=std::fopen(logpath,"ab")){
        const auto *r=glGetString(GL_RENDERER),*v=glGetString(GL_VERSION);
        std::fprintf(f,"[GodotPS5 GLES3 0.1.1] RENDERER=%s\n",r?(const char*)r:"(null)");
        std::fprintf(f,"[GodotPS5 GLES3 0.1.1] GL_VERSION=%s\n",v?(const char*)v:"(null)");
        std::fclose(f);
    }
    // Distinct static boot color, shown BEFORE the Godot renderer runs.
    glViewport(0,0,display.width(),display.height());
    glClearColor(0.03f,0.12f,0.39f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if(glGetError()!=GL_NO_ERROR||!display.swap()){record("BOOT_SWAP_FAILED");return false;}
    hui::sys::hide_splash_screen();
    record("BOOT_FRAME_PRESENTED");
    return true;
}
bool present(){
    if(!opened)return false;
    const double before=ps5_now_ms();
    const double cpu_before_swap=perf_last_swap_complete_ms>0.0
        ? std::max(0.0,before-perf_last_swap_complete_ms):0.0;
    if(!display.swap()){
        record("EGL_SWAP_FAILED",int(display.last_error()));return false;
    }
    const double after=ps5_now_ms();
    const double swap_ms=std::max(0.0,after-before);
    ++frames;
    if(frames==1){
        record("GODOT_SWAP_FIRST");
        std::printf("GODOT_PS5_M13_NATIVE_FRAME_PROFILER_READY clock=monotonic threshold_ms=80\n");
    }
    if(before>0.0 && after>0.0 && perf_last_swap_complete_ms>0.0){
        perf_report(cpu_before_swap,swap_ms,before,after);
    }
    perf_last_swap_complete_ms=after;
    if(frames==60||frames==600)record("GODOT_SWAP_COUNT",int(frames));
    return true;
}
void close_context(){
    if(!opened)return;
    record("TOTAL_GODOT_SWAP_COUNT",int(frames));
    display.close();opened=false;
    record("EGL_CLOSED");
}
void* gl_resolve(const char *name){
    return reinterpret_cast<void*>(eglGetProcAddress(name));
}
} // namespace

int main(){
    mkdir("/data/GodotPS5",0755);
    (void)freopen("/data/GodotPS5/gles3-stdout.log","ab",stdout);
    (void)freopen("/data/GodotPS5/gles3-stderr.log","ab",stderr);
    setvbuf(stdout,nullptr,_IONBF,0);setvbuf(stderr,nullptr,_IONBF,0);
    record("BOOT");
    const GodotPS5GraphicsHooks hooks={open_context,present,close_context,gl_resolve};
    godot_ps5_set_graphics_hooks(&hooks);
    constexpr char bundled[]="/app0/assets/godot/visual-smoke.pck";
    const std::string requested=godot_ps5::read_launch_txt();
    const bool external=!requested.empty() &&
        godot_ps5::trusted_external_path(requested) &&
        godot_ps5::valid_godot4_pack(requested);
    const std::string pack=external?requested:std::string(bundled);
    record_pack(external?"CONFIG":"BUNDLED_VISUAL",pack.c_str());
    record("ENTER_GODOT_GLES3");
    int result=godot_ps5_run_pack(pack.c_str());
    record("GODOT_RETURN",result);
    godot_ps5_set_graphics_hooks(nullptr);
    close_context();
    record("NORMAL_SHUTDOWN");
    hui::sys::quit();
}
