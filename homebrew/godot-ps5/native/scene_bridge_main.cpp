// SPDX-License-Identifier: GPL-3.0-or-later
// PPSA99845 — same-process GDScript-state → OpenGL presentation bridge.
// This is a diagnostic intermediary, NOT Godot's GLES3 or CanvasItem renderer.
// Godot's real SceneTree owns animation and DualSense input; native GL
// displays a procedural preview using its actual per-frame scene transforms.
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/system.hpp"
#include <GL/glcorearb.h>
#include <array>
#include <cstdio>
#include <cmath>
#include <sys/stat.h>

extern "C" int godot_ps5_run_pack(const char *pack);
extern "C" void godot_ps5_set_scene_frame_callback(void (*)(float, float, int));

namespace {
constexpr char kLog[]="/data/GodotPS5/scene-bridge.log";
void log(const char *name,int value=-1) {
    if(FILE*f=std::fopen(kLog,"ab")){
        if(value>=0)std::fprintf(f,"[GodotPS5 BRIDGE 0.1.0] %s=%d\n",name,value);
        else std::fprintf(f,"[GodotPS5 BRIDGE 0.1.0] %s\n",name);
        std::fflush(f); std::fclose(f);
    }
}
GLuint compile(GLenum type,const char*source){
    GLuint shader=glCreateShader(type);
    if(!shader)return 0;
    glShaderSource(shader,1,&source,nullptr);
    glCompileShader(shader);
    GLint status=GL_FALSE;
    glGetShaderiv(shader,GL_COMPILE_STATUS,&status);
    if(status!=GL_TRUE){
        char buffer[1024]={};GLsizei size=0;
        glGetShaderInfoLog(shader,sizeof(buffer)-1,&size,buffer);
        if(FILE*f=std::fopen(kLog,"ab")){
            std::fprintf(f,"[GodotPS5 BRIDGE 0.1.0] SHADER_ERROR=%s\n",buffer);
            std::fclose(f);
        }
        glDeleteShader(shader);return 0;
    }
    return shader;
}
constexpr char kVertex[]=R"(#version 460 core
layout(location=0) in vec2 pos;
layout(location=1) in vec2 uv;
uniform vec2 spritePos;
uniform float angle;
out vec2 uvFrag;
void main() {
    float s=sin(angle), c=cos(angle);
    vec2 rotated=vec2(c*pos.x-s*pos.y,s*pos.x+c*pos.y);
    gl_Position=vec4(rotated+spritePos,0,1);
    uvFrag=uv;
}
)";
constexpr char kFragment[]=R"(#version 460 core
in vec2 uvFrag;
uniform sampler2D spriteTexture;
layout(location=0) out vec4 color;
void main() {color=texture(spriteTexture,uvFrag);}
)";
struct GLScene {
    hui::ps5::Display display;
    GLuint program=0,vao=0,vbo=0,texture=0;
    GLint position=-1,rotation=-1,sampler=-1;
    int frames=0, last_presses=-1;
    bool ready=false;
    bool open(){
        if(!display.open(1920,1080)){log("EGL_OPEN_FAILED",display.last_error());return false;}
        log("EGL_OPEN_OK");
        GLuint vs=compile(GL_VERTEX_SHADER,kVertex),fs=compile(GL_FRAGMENT_SHADER,kFragment);
        if(!vs||!fs){log("GL_SHADER_FAILURE");if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);return false;}
        program=glCreateProgram();
        glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);
        glDeleteShader(vs);glDeleteShader(fs);
        GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
        if(!ok){log("GL_PROGRAM_LINK_FAILED");return false;}
        position=glGetUniformLocation(program,"spritePos");
        rotation=glGetUniformLocation(program,"angle");
        sampler=glGetUniformLocation(program,"spriteTexture");
        if(position<0||rotation<0||sampler<0){log("GL_UNIFORM_FAILURE");return false;}
        constexpr int side=64;
        std::array<unsigned char,side*side*4> pixels{};
        for(int y=0;y<side;++y)for(int x=0;x<side;++x){
            const int off=(y*side+x)*4;
            const bool on=((x/8+y/8)&1)==0;
            pixels[off]=on?255:10;
            pixels[off+1]=on?35:235;
            pixels[off+2]=on?160:255;
            pixels[off+3]=180;
        }
        glGenTextures(1,&texture);
        glBindTexture(GL_TEXTURE_2D,texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,side,side,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        glBindTexture(GL_TEXTURE_2D,0);
        constexpr GLfloat quad[]={
            -0.28f,-0.50f,0,0,
             0.28f,-0.50f,1,0,
            -0.28f, 0.50f,0,1,
             0.28f, 0.50f,1,1
        };
        glGenVertexArrays(1,&vao);
        glBindVertexArray(vao);
        glGenBuffers(1,&vbo);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),(const void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),(const void*)(2*sizeof(GLfloat)));
        glBindVertexArray(0);
        if(glGetError()!=GL_NO_ERROR){log("GL_SETUP_ERROR");return false;}
        ready=true;log("SCENE_BRIDGE_GL_READY");
        return true;
    }
    void frame(float sprite_x,float angle,int presses){
        if(!ready)return;
        if(presses!=last_presses){
            log("GDSCRIPT_CROSS_COUNT",presses);last_presses=presses;
        }
        // Count is read directly from a live Godot GDScript scene property;
        // NO independent native pad polling or second animation clock.
        const bool red=(presses&1)!=0;
        glViewport(0,0,display.width(),display.height());
        glClearColor(red?0.18f:0.06f,red?0.07f:0.10f,red?0.08f:0.18f,1);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(program);
        glUniform2f(position,(sprite_x-960.0f)/960.0f,0.0f);
        glUniform1f(rotation,angle);
        glUniform1i(sampler,0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,texture);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D,0);
        glUseProgram(0);
        glDisable(GL_BLEND);
        if(frames==0){
            log("GODOT_GDSCRIPT_SCENE_FRAME_DRAWN");
            log("FIRST_DRAW_GL_ERROR",(int)glGetError());
        }
        if(!display.swap()){log("EGL_SWAP_FAILED",display.last_error());ready=false;return;}
        ++frames;
        if(frames==1){log("FIRST_FRAME_PRESENTED");hui::sys::hide_splash_screen();}
        if(frames==60||frames==600)log("PRESENTED_FRAME_COUNT",frames);
    }
    void close(){
        log("TOTAL_PRESENTED",frames);
        if(vbo)glDeleteBuffers(1,&vbo);
        if(vao)glDeleteVertexArrays(1,&vao);
        if(texture)glDeleteTextures(1,&texture);
        if(program)glDeleteProgram(program);
        display.close();
        log("GPU_SHUTDOWN");
    }
};
GLScene* active=nullptr;
void scene_callback(float x,float rotation,int count){
    if(active)active->frame(x,rotation,count);
}
} // namespace
int main(){
    mkdir("/data/GodotPS5",0755);
    log("BOOT_SCENE_BRIDGE");
    FILE *out=freopen("/data/GodotPS5/bridge-stdout.log","ab",stdout);
    FILE *err=freopen("/data/GodotPS5/bridge-stderr.log","ab",stderr);
    if(out)setvbuf(stdout,nullptr,_IONBF,0);
    if(err)setvbuf(stderr,nullptr,_IONBF,0);
    GLScene scene;
    if(!scene.open()){log("GPU_INIT_FAILED");scene.close();hui::sys::quit();}
    active=&scene;
    godot_ps5_set_scene_frame_callback(scene_callback);
    log("GODOT_ENGINE_ENTER");
    const int result=godot_ps5_run_pack("/app0/assets/godot/visual-smoke.pck");
    log("GODOT_ENGINE_RETURN",result);
    godot_ps5_set_scene_frame_callback(nullptr);
    active=nullptr;
    scene.close();
    log("NORMAL_SHUTDOWN");
    hui::sys::quit();
}
