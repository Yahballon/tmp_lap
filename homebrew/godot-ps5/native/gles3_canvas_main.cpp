// SPDX-License-Identifier: GPL-3.0-or-later
// PPSA99847: real Godot 4.7.2 GLES3 RenderingServer canvas test.
// No native sprite shaders or glDrawArrays: Godot must draw the scene.
#include "graphics_hooks_ps5.h"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/system.hpp"
#include <EGL/egl.h>
#include <GL/glcorearb.h>
#include <cstdio>
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
    if(!display.swap()){
        record("EGL_SWAP_FAILED",int(display.last_error()));return false;
    }
    ++frames;
    if(frames==1)record("GODOT_SWAP_FIRST");
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
