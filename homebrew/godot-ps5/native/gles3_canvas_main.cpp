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
#include <atomic>
#include <cstring>
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
// M15: safe PS5 EGL present pacing; immediate remains opt-in for A/B diagnosis.
// The confirmed M12d game pack is untouched. Experimental TESTBUILD defaults
// to immediate presents to compare with the original VSync=1 pipeline.
// Write "vsync" to /data/GodotPS5/present_mode.txt to restore the baseline.
enum class PS5PresentMode { Immediate, VSync };
PS5PresentMode perf_present_mode = PS5PresentMode::VSync;
const char *perf_present_mode_name(PS5PresentMode mode) {
    return mode == PS5PresentMode::Immediate ? "immediate" : "vsync";
}
void load_present_mode() {
    const char *conf="/data/GodotPS5/present_mode.txt";
    const int fd=sceKernelOpen(conf,O_RDONLY | O_NOFOLLOW,0);
    if(fd<0) {
        std::printf("GODOT_PS5_M14_CONFIG file=absent mode=vsync (safe_default)\n");
        return;
    }
    char data[32]{};
    const long n=sceKernelRead(fd,data,sizeof(data));
    sceKernelClose(fd);
    if(n<=0 || static_cast<size_t>(n)>=sizeof(data)) {
        perf_present_mode=PS5PresentMode::VSync;
        std::printf("GODOT_PS5_M14_CONFIG file=invalid mode=vsync\n");
        return;
    }
    std::string mode(data,static_cast<size_t>(n));
    const size_t first=mode.find_first_not_of(" \t\r\n");
    if(first==std::string::npos)mode.clear();
    else mode=mode.substr(first,mode.find_last_not_of(" \t\r\n")-first+1);
    if(mode=="immediate")perf_present_mode=PS5PresentMode::Immediate;
    else perf_present_mode=PS5PresentMode::VSync;
    std::printf("GODOT_PS5_M14_CONFIG file=present mode=%s\n",perf_present_mode_name(perf_present_mode));
}
void apply_present_mode() {
    // The DevBridge Display::open has already established VSync=1. Only
    // an explicit, successful EGL call overrides that known-good default.
    const EGLDisplay d=eglGetCurrentDisplay();
    const bool request_fast=perf_present_mode==PS5PresentMode::Immediate;
    bool changed=false;
    EGLint error=EGL_SUCCESS;
    if(request_fast && d!=EGL_NO_DISPLAY) {
        changed=eglSwapInterval(d,0)==EGL_TRUE;
        if(!changed) {
            error=eglGetError();
            // Fail back to the known-good interval 1 if unsupported.
            (void)eglSwapInterval(d,1);
        }
    }
    const char *effective=request_fast && changed ? "immediate" : "vsync";
    std::printf("GODOT_PS5_M14_PRESENT_MODE requested=%s effective=%s egl_error=0x%x\n",
        perf_present_mode_name(perf_present_mode),effective,static_cast<unsigned>(error));
}

// M19 – low-overhead timing of selected Godot GLES3 OpenGL entry points.
// Each API call is delegated without changing GL state or order.
// A timer may itself slightly perturb render speed; M19 is diagnostic only.
// Resolved function pointers are recorded after the current EGL context opens.
struct M19GlStat {
    const char *name;
    std::atomic<unsigned long long> calls{0}, total_us{0}, max_us{0}, spikes_80ms{0};
};
M19GlStat m19_gl_stats[] = {
    {"glDrawElements"},
    {"glDrawArrays"},
    {"glDrawElementsInstanced"},
    {"glDrawArraysInstanced"},
    {"glBufferData"},
    {"glBufferSubData"},
    {"glMapBufferRange"},
    {"glUnmapBuffer"},
    {"glTexImage2D"},
    {"glTexSubImage2D"},
    {"glFenceSync"},
    {"glClientWaitSync"},
    {"glFinish"},
    {"glFlush"},
    {"glClear"},
    {"glBlitFramebuffer"},
    {"glReadPixels"},
    {"glGetError"},
    {"glBindFramebuffer"},
    {"glUseProgram"},
    {"glBindBuffer"},
    {"glGetIntegerv"},
    {"glGetUniformLocation"},
};
constexpr size_t m19_gl_count=sizeof(m19_gl_stats)/sizeof(m19_gl_stats[0]);
double m19_window_start_ms=0.0;
unsigned m19_hooked_count=0;
void m19_add(size_t idx, double elapsed_ms) {
    const auto us=static_cast<unsigned long long>(std::max(0.0,elapsed_ms)*1000.0);
    auto &s=m19_gl_stats[idx];
    s.calls.fetch_add(1,std::memory_order_relaxed);
    s.total_us.fetch_add(us,std::memory_order_relaxed);
    if(us>=80000)s.spikes_80ms.fetch_add(1,std::memory_order_relaxed);
    auto previous=s.max_us.load(std::memory_order_relaxed);
    while(previous<us && !s.max_us.compare_exchange_weak(
        previous,us,std::memory_order_relaxed,std::memory_order_relaxed)) {}
}
void m19_report(double now_ms) {
    if(m19_window_start_ms==0.0) {
        m19_window_start_ms=now_ms;
        std::printf("GODOT_PS5_M19_GL_PROFILER_READY hooked=%u names=%zu threshold_ms=80\n",
            m19_hooked_count,m19_gl_count);
    }
    if(now_ms-m19_window_start_ms<5000.0)return;
    unsigned long long all_calls=0,all_us=0,max_us=0,all_spikes=0;
    for(size_t i=0;i<m19_gl_count;++i) {
        auto &s=m19_gl_stats[i];
        const auto calls=s.calls.exchange(0,std::memory_order_relaxed);
        const auto total=s.total_us.exchange(0,std::memory_order_relaxed);
        const auto peak=s.max_us.exchange(0,std::memory_order_relaxed);
        const auto spikes=s.spikes_80ms.exchange(0,std::memory_order_relaxed);
        all_calls+=calls;all_us+=total;all_spikes+=spikes;
        if(peak>max_us)max_us=peak;
        if(calls>0) {
            std::printf("GODOT_PS5_M19_GL_5S name=%s calls=%llu total_ms=%.2f max_call_ms=%.2f spikes_80ms=%llu\n",
                s.name,calls,total/1000.0,peak/1000.0,spikes);
        }
    }
    std::printf("GODOT_PS5_M19_GL_SUMMARY_5S hooked=%u calls=%llu total_ms=%.2f max_call_ms=%.2f spikes_80ms=%llu\n",
        m19_hooked_count,all_calls,all_us/1000.0,max_us/1000.0,all_spikes);
    m19_window_start_ms=now_ms;
}
PFNGLDRAWELEMENTSPROC m19_real_glDrawElements=nullptr;
void m19_trace_glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    const double before=ps5_now_ms();
    m19_real_glDrawElements(mode,count,type,indices);
    m19_add(0,ps5_now_ms()-before);
}
PFNGLDRAWARRAYSPROC m19_real_glDrawArrays=nullptr;
void m19_trace_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    const double before=ps5_now_ms();
    m19_real_glDrawArrays(mode,first,count);
    m19_add(1,ps5_now_ms()-before);
}
PFNGLDRAWELEMENTSINSTANCEDPROC m19_real_glDrawElementsInstanced=nullptr;
void m19_trace_glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount) {
    const double before=ps5_now_ms();
    m19_real_glDrawElementsInstanced(mode,count,type,indices,instancecount);
    m19_add(2,ps5_now_ms()-before);
}
PFNGLDRAWARRAYSINSTANCEDPROC m19_real_glDrawArraysInstanced=nullptr;
void m19_trace_glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
    const double before=ps5_now_ms();
    m19_real_glDrawArraysInstanced(mode,first,count,instancecount);
    m19_add(3,ps5_now_ms()-before);
}
PFNGLBUFFERDATAPROC m19_real_glBufferData=nullptr;
void m19_trace_glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    const double before=ps5_now_ms();
    m19_real_glBufferData(target,size,data,usage);
    m19_add(4,ps5_now_ms()-before);
}
PFNGLBUFFERSUBDATAPROC m19_real_glBufferSubData=nullptr;
void m19_trace_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    const double before=ps5_now_ms();
    m19_real_glBufferSubData(target,offset,size,data);
    m19_add(5,ps5_now_ms()-before);
}
PFNGLMAPBUFFERRANGEPROC m19_real_glMapBufferRange=nullptr;
void* m19_trace_glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    const double before=ps5_now_ms();
    auto result=m19_real_glMapBufferRange(target,offset,length,access);
    m19_add(6,ps5_now_ms()-before);
    return result;
}
PFNGLUNMAPBUFFERPROC m19_real_glUnmapBuffer=nullptr;
GLboolean m19_trace_glUnmapBuffer(GLenum target) {
    const double before=ps5_now_ms();
    auto result=m19_real_glUnmapBuffer(target);
    m19_add(7,ps5_now_ms()-before);
    return result;
}
PFNGLTEXIMAGE2DPROC m19_real_glTexImage2D=nullptr;
void m19_trace_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
    const double before=ps5_now_ms();
    m19_real_glTexImage2D(target,level,internalformat,width,height,border,format,type,pixels);
    m19_add(8,ps5_now_ms()-before);
}
PFNGLTEXSUBIMAGE2DPROC m19_real_glTexSubImage2D=nullptr;
void m19_trace_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    const double before=ps5_now_ms();
    m19_real_glTexSubImage2D(target,level,xoffset,yoffset,width,height,format,type,pixels);
    m19_add(9,ps5_now_ms()-before);
}
PFNGLFENCESYNCPROC m19_real_glFenceSync=nullptr;
GLsync m19_trace_glFenceSync(GLenum condition, GLbitfield flags) {
    const double before=ps5_now_ms();
    auto result=m19_real_glFenceSync(condition,flags);
    m19_add(10,ps5_now_ms()-before);
    return result;
}
PFNGLCLIENTWAITSYNCPROC m19_real_glClientWaitSync=nullptr;
GLenum m19_trace_glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    const double before=ps5_now_ms();
    auto result=m19_real_glClientWaitSync(sync,flags,timeout);
    m19_add(11,ps5_now_ms()-before);
    return result;
}
PFNGLFINISHPROC m19_real_glFinish=nullptr;
void m19_trace_glFinish(void) {
    const double before=ps5_now_ms();
    m19_real_glFinish();
    m19_add(12,ps5_now_ms()-before);
}
PFNGLFLUSHPROC m19_real_glFlush=nullptr;
void m19_trace_glFlush(void) {
    const double before=ps5_now_ms();
    m19_real_glFlush();
    m19_add(13,ps5_now_ms()-before);
}
PFNGLCLEARPROC m19_real_glClear=nullptr;
void m19_trace_glClear(GLbitfield mask) {
    const double before=ps5_now_ms();
    m19_real_glClear(mask);
    m19_add(14,ps5_now_ms()-before);
}
PFNGLBLITFRAMEBUFFERPROC m19_real_glBlitFramebuffer=nullptr;
void m19_trace_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    const double before=ps5_now_ms();
    m19_real_glBlitFramebuffer(srcX0,srcY0,srcX1,srcY1,dstX0,dstY0,dstX1,dstY1,mask,filter);
    m19_add(15,ps5_now_ms()-before);
}
PFNGLREADPIXELSPROC m19_real_glReadPixels=nullptr;
void m19_trace_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
    const double before=ps5_now_ms();
    m19_real_glReadPixels(x,y,width,height,format,type,pixels);
    m19_add(16,ps5_now_ms()-before);
}
PFNGLGETERRORPROC m19_real_glGetError=nullptr;
GLenum m19_trace_glGetError(void) {
    const double before=ps5_now_ms();
    auto result=m19_real_glGetError();
    m19_add(17,ps5_now_ms()-before);
    return result;
}
PFNGLBINDFRAMEBUFFERPROC m19_real_glBindFramebuffer=nullptr;
void m19_trace_glBindFramebuffer(GLenum target, GLuint framebuffer) {
    const double before=ps5_now_ms();
    m19_real_glBindFramebuffer(target,framebuffer);
    m19_add(18,ps5_now_ms()-before);
}
PFNGLUSEPROGRAMPROC m19_real_glUseProgram=nullptr;
void m19_trace_glUseProgram(GLuint program) {
    const double before=ps5_now_ms();
    m19_real_glUseProgram(program);
    m19_add(19,ps5_now_ms()-before);
}
PFNGLBINDBUFFERPROC m19_real_glBindBuffer=nullptr;
void m19_trace_glBindBuffer(GLenum target, GLuint buffer) {
    const double before=ps5_now_ms();
    m19_real_glBindBuffer(target,buffer);
    m19_add(20,ps5_now_ms()-before);
}
PFNGLGETINTEGERVPROC m19_real_glGetIntegerv=nullptr;
void m19_trace_glGetIntegerv(GLenum pname, GLint *data) {
    const double before=ps5_now_ms();
    m19_real_glGetIntegerv(pname,data);
    m19_add(21,ps5_now_ms()-before);
}
PFNGLGETUNIFORMLOCATIONPROC m19_real_glGetUniformLocation=nullptr;
GLint m19_trace_glGetUniformLocation(GLuint program, const GLchar *name) {
    const double before=ps5_now_ms();
    auto result=m19_real_glGetUniformLocation(program,name);
    m19_add(22,ps5_now_ms()-before);
    return result;
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
    apply_present_mode();
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
    m19_report(after);
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
    if(!name)return nullptr;
    void *original=reinterpret_cast<void*>(eglGetProcAddress(name));
    if(!original)return nullptr;
    if(std::strcmp(name,"glDrawElements")==0) {
        m19_real_glDrawElements=reinterpret_cast<PFNGLDRAWELEMENTSPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glDrawElements);
    }
    if(std::strcmp(name,"glDrawArrays")==0) {
        m19_real_glDrawArrays=reinterpret_cast<PFNGLDRAWARRAYSPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glDrawArrays);
    }
    if(std::strcmp(name,"glDrawElementsInstanced")==0) {
        m19_real_glDrawElementsInstanced=reinterpret_cast<PFNGLDRAWELEMENTSINSTANCEDPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glDrawElementsInstanced);
    }
    if(std::strcmp(name,"glDrawArraysInstanced")==0) {
        m19_real_glDrawArraysInstanced=reinterpret_cast<PFNGLDRAWARRAYSINSTANCEDPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glDrawArraysInstanced);
    }
    if(std::strcmp(name,"glBufferData")==0) {
        m19_real_glBufferData=reinterpret_cast<PFNGLBUFFERDATAPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glBufferData);
    }
    if(std::strcmp(name,"glBufferSubData")==0) {
        m19_real_glBufferSubData=reinterpret_cast<PFNGLBUFFERSUBDATAPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glBufferSubData);
    }
    if(std::strcmp(name,"glMapBufferRange")==0) {
        m19_real_glMapBufferRange=reinterpret_cast<PFNGLMAPBUFFERRANGEPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glMapBufferRange);
    }
    if(std::strcmp(name,"glUnmapBuffer")==0) {
        m19_real_glUnmapBuffer=reinterpret_cast<PFNGLUNMAPBUFFERPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glUnmapBuffer);
    }
    if(std::strcmp(name,"glTexImage2D")==0) {
        m19_real_glTexImage2D=reinterpret_cast<PFNGLTEXIMAGE2DPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glTexImage2D);
    }
    if(std::strcmp(name,"glTexSubImage2D")==0) {
        m19_real_glTexSubImage2D=reinterpret_cast<PFNGLTEXSUBIMAGE2DPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glTexSubImage2D);
    }
    if(std::strcmp(name,"glFenceSync")==0) {
        m19_real_glFenceSync=reinterpret_cast<PFNGLFENCESYNCPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glFenceSync);
    }
    if(std::strcmp(name,"glClientWaitSync")==0) {
        m19_real_glClientWaitSync=reinterpret_cast<PFNGLCLIENTWAITSYNCPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glClientWaitSync);
    }
    if(std::strcmp(name,"glFinish")==0) {
        m19_real_glFinish=reinterpret_cast<PFNGLFINISHPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glFinish);
    }
    if(std::strcmp(name,"glFlush")==0) {
        m19_real_glFlush=reinterpret_cast<PFNGLFLUSHPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glFlush);
    }
    if(std::strcmp(name,"glClear")==0) {
        m19_real_glClear=reinterpret_cast<PFNGLCLEARPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glClear);
    }
    if(std::strcmp(name,"glBlitFramebuffer")==0) {
        m19_real_glBlitFramebuffer=reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glBlitFramebuffer);
    }
    if(std::strcmp(name,"glReadPixels")==0) {
        m19_real_glReadPixels=reinterpret_cast<PFNGLREADPIXELSPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glReadPixels);
    }
    if(std::strcmp(name,"glGetError")==0) {
        m19_real_glGetError=reinterpret_cast<PFNGLGETERRORPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glGetError);
    }
    if(std::strcmp(name,"glBindFramebuffer")==0) {
        m19_real_glBindFramebuffer=reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glBindFramebuffer);
    }
    if(std::strcmp(name,"glUseProgram")==0) {
        m19_real_glUseProgram=reinterpret_cast<PFNGLUSEPROGRAMPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glUseProgram);
    }
    if(std::strcmp(name,"glBindBuffer")==0) {
        m19_real_glBindBuffer=reinterpret_cast<PFNGLBINDBUFFERPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glBindBuffer);
    }
    if(std::strcmp(name,"glGetIntegerv")==0) {
        m19_real_glGetIntegerv=reinterpret_cast<PFNGLGETINTEGERVPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glGetIntegerv);
    }
    if(std::strcmp(name,"glGetUniformLocation")==0) {
        m19_real_glGetUniformLocation=reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(original);
        ++m19_hooked_count;
        return reinterpret_cast<void*>(&m19_trace_glGetUniformLocation);
    }
    return original;
}
} // namespace

int main(){
    mkdir("/data/GodotPS5",0755);
    (void)freopen("/data/GodotPS5/gles3-stdout.log","ab",stdout);
    (void)freopen("/data/GodotPS5/gles3-stderr.log","ab",stderr);
    setvbuf(stdout,nullptr,_IONBF,0);setvbuf(stderr,nullptr,_IONBF,0);
    record("BOOT");
    load_present_mode();
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
