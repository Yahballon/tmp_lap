// AndroidFlow by YAHBALLON — Hydroid native PS5 APK launcher.
// Uses Hydroid game scanner/GL renderer and GBAXY-inspired Coverflow.
#include "yah_coverflow.h"
#include "yah_coverflow_math.h"
#include "ui_gl.h"
#include "ui_font.h"
#include "android_shim.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_GAMES LIB_MAX
#define BTN_X 0x4000u
#define BTN_O 0x2000u
#define BTN_TRI 0x1000u
#define BTN_L1 0x400u
#define BTN_R1 0x800u
#define BTN_LEFT 0x80u
#define BTN_RIGHT 0x20u

void droid_menu_pad(uint32_t *buttons, uint8_t *lx, uint8_t *ly);
void droid_pad_rumble(float strength, int ms);
void droid_log(const char *fmt, ...);

static lib_game games[MAX_GAMES];
static unsigned textures[MAX_GAMES];
static int count, selected;
static float target, phase;
static const char *roots[]={
  APP0 "/game", "/mnt/usb0/Android", "/mnt/usb1/Android", "/data/AndroidFlow/apks"
};
static double now_sec(void){
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC,&t);
  return t.tv_sec+t.tv_nsec/1e9;
}
static int by_name(const void *a,const void *b){
  return strcasecmp(((const lib_game*)a)->name,((const lib_game*)b)->name);
}
static void release_library(void){
  for(int i=0;i<count;i++){
    if(textures[i])ui_texture_free(textures[i]);
    textures[i]=0;
  }
  library_free(games,count);
  count=0;
}
static void scan_library(void){
  release_library();
  for(size_t i=0;i<sizeof(roots)/sizeof(roots[0]) && count<MAX_GAMES;i++){
    struct stat st;
    if(stat(roots[i],&st)!=0 || !S_ISDIR(st.st_mode))continue;
    int added=library_scan_in(roots[i],games+count,MAX_GAMES-count);
    if(added>0)count+=added;
  }
  if(count)qsort(games,(size_t)count,sizeof(games[0]),by_name);
  selected=0;
  target=phase=0.f;
  droid_log("AndroidFlow: found %d APK files",count);
}
static void draw_card(int i,float offset,float now,int focused){
  yah_pose p=yah_cover_pose(offset,now);
  const float w=270.f,h=368.f,x=p.x-135.f,y=299.f;
  float alpha=p.brightness;
  ui_transform(p.x,y+h*.5f,0.f,p.yaw,p.depth,p.scale);
  if(focused)ui_rect(x-9.f,y-10.f,w+18.f,h+20.f,32.f,19.f,
                     UI_RGBA(52,230,211,0.42f));
  ui_rect_gradient(x,y,w,h,25.f,UI_RGBA(42,57,82,alpha),
                   UI_RGBA(24,30,47,alpha));
  ui_rect(x+20.f,y+16.f,w-40.f,4.f,2.f,1.f,
          UI_RGBA(98,232,214,alpha*(focused?1.f:0.4f)));
  ui_rect(x+25.f,y+48.f,w-50.f,220.f,26.f,1.f,UI_RGBA(9,16,31,.80f));
  if(games[i].icon && games[i].icon_size>0){
    if(!textures[i])textures[i]=ui_texture_rgba(games[i].icon,
                                               games[i].icon_size,games[i].icon_size);
    if(textures[i])ui_image_shine(textures[i],x+39.f,y+62.f,192.f,192.f,28.f,
                                  alpha,.25f+.3f*sinf(now),focused?.16f:0.f);
  }else{
    ui_rect_gradient(x+39.f,y+62.f,192.f,192.f,27.f,
                     UI_RGBA(37,96,109,alpha),UI_RGBA(20,43,69,alpha));
    ui_text(UI_TEXT_TITLE,p.x,y+133.f,"APK",UI_RGBA(218,255,255,alpha),
            UI_ALIGN_CENTER);
  }
  ui_text_fit(UI_TEXT_LABEL,p.x,y+280.f,w-26.f,games[i].name,
              UI_RGBA(249,252,255,alpha),UI_ALIGN_CENTER);
  ui_text_fit(UI_TEXT_SMALL,p.x,y+322.f,w-38.f,
              games[i].engine[0]?games[i].engine:"Android ARM64",
              UI_RGBA(151,178,200,alpha),UI_ALIGN_CENTER);
  ui_transform_reset();
}
static void draw_frame(double t){
  ui_frame_begin();
  ui_rect_gradient(0,0,UI_W,UI_H,0,UI_RGBA(8,13,26,1),UI_RGBA(20,33,53,1));
  ui_rect(76,82,9,69,4,1,UI_RGBA(67,231,209,1));
  ui_text(UI_TEXT_TITLE,109,85,"ANDROID FLOW",UI_RGBA(242,249,255,1),UI_ALIGN_LEFT);
  ui_text(UI_TEXT_SMALL,111,144,"by YAHBALLON   /   Powered by Hydroid",
          UI_RGBA(146,179,195,1),UI_ALIGN_LEFT);
  ui_text(UI_TEXT_LABEL,1816,105,"v0.1.0",UI_RGBA(148,173,185,1),UI_ALIGN_RIGHT);
  ui_rect_gradient(0,808,UI_W,272,0,UI_RGBA(12,21,37,.4f),
                   UI_RGBA(6,10,21,1));
  if(!count){
    ui_text(UI_TEXT_TITLE,960,431,"NO ANDROID APPS FOUND",
            UI_RGBA(236,246,251,1),UI_ALIGN_CENTER);
    ui_text(UI_TEXT_BODY,960,505,"Copy .apk, .xapk, .apks or .apkm files to:",
            UI_RGBA(164,185,202,1),UI_ALIGN_CENTER);
    ui_text(UI_TEXT_BODY,960,557,"/data/homebrew/PPSA99920/game/ or USB/Android/",
            UI_RGBA(86,231,210,1),UI_ALIGN_CENTER);
  }else{
    // Draw distant cards first, selected card last (overlaps and focus correct).
    for(int layer=4;layer>=0;layer--)for(int i=0;i<count;i++){
      float offset=yah_wrap_offset(i-phase,count),dist=fabsf(offset);
      if(dist>=layer-.5f && dist<layer+.5f)
        draw_card(i,offset,(float)t,i==selected);
    }
    ui_text_fit(UI_TEXT_HUGE,960,826,1250,games[selected].name,
                UI_RGBA(245,251,255,1),UI_ALIGN_CENTER);
    ui_text_fit(UI_TEXT_SMALL,960,913,1550,games[selected].path,
                UI_RGBA(139,173,188,1),UI_ALIGN_CENTER);
  }
  char info[64];
  snprintf(info,sizeof(info),"%d APP%s",count,count==1?"":"S");
  ui_text(UI_TEXT_LABEL,103,998,info,UI_RGBA(94,220,201,1),UI_ALIGN_LEFT);
  ui_text(UI_TEXT_SMALL,1823,999,
          "LEFT/RIGHT BROWSE    X LAUNCH    TRIANGLE RESCAN    CIRCLE EXIT",
          UI_RGBA(203,222,235,1),UI_ALIGN_RIGHT);
  ui_frame_end();
}
int yah_coverflow_pick(lib_game *picked){
  if(!picked || ui_open()!=0)return -2;
  if(ui_font_init(APP0 "/assets/fonts")!=0)
    droid_log("AndroidFlow: font init failed");
  scan_library();
  uint32_t previous=0;
  int held_dir=0;
  double next_repeat=0,last=now_sec();
  int result=-1;
  for(;;){
    double now=now_sec();
    float dt=yah_clamp((float)(now-last),0.f,.05f);
    last=now;
    uint32_t buttons=0;
    uint8_t lx=128,ly=128;
    droid_menu_pad(&buttons,&lx,&ly);
    (void)ly;
    int dir=((buttons & BTN_RIGHT)||lx>=190)?1:
            ((buttons & BTN_LEFT)||lx<=65)?-1:0;
    if(count && dir && (dir!=held_dir || now>=next_repeat)){
      selected=yah_wrap(selected+dir,count);
      target+=dir;
      next_repeat=now+(dir==held_dir?.16:.28);
      droid_pad_rumble(.10f,24);
    }
    held_dir=dir;
    uint32_t pressed=buttons & ~previous;
    previous=buttons;
    if((pressed & BTN_L1)&&count){selected=yah_wrap(selected-5,count);target-=5;}
    if((pressed & BTN_R1)&&count){selected=yah_wrap(selected+5,count);target+=5;}
    if(pressed & BTN_TRI)scan_library();
    if(pressed & BTN_O)break;
    if((pressed & BTN_X)&&count){
      *picked=games[selected];
      games[selected].icon=NULL; // transfer selected icon memory to caller
      result=0;
      break;
    }
    phase+=(target-phase)*yah_clamp(dt*14.f,0.f,1.f);
    draw_frame(now);
  }
  release_library();
  ui_close(); // release video output before the game's own OpenGL/Vulkan context
  return result;
}
