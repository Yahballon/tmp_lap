// SPDX-License-Identifier: MIT
// FITBREW 0.1.0 by YAHBALLON - original equipment-free fitness app.
#include "workout.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "vendor/stb_image.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
using namespace hui::gfx;
using namespace fitbrew;
namespace {
constexpr unsigned LIME=0xd9ff00,WHITE=0xf8fafc,MUTED=0x9faabd,DARK=0x080d14,EDGE=0x303a49;
Color c(unsigned v,float a=1){return Color::rgb(v,a);}
void logs(const char* msg){
 mkdir("/data/FITBREW",0755);mkdir("/data/FITBREW/logs",0755);
 if(FILE* f=fopen("/data/FITBREW/logs/fitbrew.log","ab")){
  fprintf(f,"[FITBREW 0.1.0] %s\n",msg);fclose(f);
 }
}
std::string read_file(const std::string& path,size_t max=12*1024*1024){
 FILE* f=fopen(path.c_str(),"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)!=0){fclose(f);return {};}
 const long n=ftell(f);if(n<=0||size_t(n)>max){fclose(f);return {};}
 rewind(f);std::string data(size_t(n),'\0');
 if(fread(data.data(),1,data.size(),f)!=data.size())data.clear();
 fclose(f);return data;
}
std::string asset(const std::string& name,size_t max=12*1024*1024){
 for(const char* p:{"/app0/assets/","/data/homebrew/PPSA99790/assets/","/mnt/usb0/homebrew/PPSA99790/assets/","assets/"}){
  auto bytes=read_file(std::string(p)+name,max);if(!bytes.empty())return bytes;
 }return {};
}
void load_progress(State& s){
 const auto data=read_file("/data/FITBREW/progress.ini",16384);
 if(data.empty())return;
 int ver=0,finished=0,seconds=0,streak=0,level=0;
 long day=-1;
 int week[7]={};
 const int got=sscanf(data.c_str(),
  "v=%d\ncompleted=%d\nseconds=%d\nstreak=%d\nlast_day=%ld\nlevel=%d\nweek=%d,%d,%d,%d,%d,%d,%d",
  &ver,&finished,&seconds,&streak,&day,&level,
  &week[0],&week[1],&week[2],&week[3],&week[4],&week[5],&week[6]);
 if(got!=13||ver!=1){logs("Progress invalid: keeping clean state");return;}
 s.history.completed=std::clamp(finished,0,1000000);
 s.history.seconds=std::clamp(seconds,0,2000000000);
 s.history.streak=std::clamp(streak,0,1000000);
 s.history.last_day=day; s.level=std::clamp(level,0,2);
 for(int i=0;i<7;i++)s.history.week[i]=std::clamp(week[i],0,10000000);
}
void save_progress(const State& s){
 mkdir("/data/FITBREW",0755);
 FILE* f=fopen("/data/FITBREW/progress.tmp","wb");if(!f){logs("Progress write failed");return;}
 const auto& h=s.history;
 fprintf(f,"v=1\ncompleted=%d\nseconds=%d\nstreak=%d\nlast_day=%ld\nlevel=%d\nweek=%d,%d,%d,%d,%d,%d,%d",
  h.completed,h.seconds,h.streak,h.last_day,s.level,
  h.week[0],h.week[1],h.week[2],h.week[3],h.week[4],h.week[5],h.week[6]);
 if(fclose(f)==0){if(rename("/data/FITBREW/progress.tmp","/data/FITBREW/progress.ini")!=0)logs("Progress rename failed");}
 else logs("Progress close failed");
}
unsigned load_image(Renderer& renderer,const std::string& name){
 const auto bytes=asset(name);if(bytes.empty()){logs("Missing art asset");return 0;}
 int w=0,h=0,components=0;
 auto* rgba=stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()),int(bytes.size()),&w,&h,&components,4);
 if(!rgba||w<=0||h<=0||w>2048||h>2048){if(rgba)stbi_image_free(rgba);logs("Image decode failed");return 0;}
 const unsigned texture=renderer.batch().create_texture(w,h,rgba);
 stbi_image_free(rgba);return texture;
}
void text(DrawList& d,const Font& f,unsigned atlas,const std::string& value,float x,float y,float size,unsigned color=WHITE,Align align=Align::left){
 d.text(f,atlas,value,x,y,size,c(color),align);
}
void panel(DrawList& d,float x,float y,float w,float h,bool selected=false){
 d.bordered_rect({x,y,w,h},19,c(0x121a26,.96f),selected?3.f:1.3f,c(selected?LIME:EDGE));
 if(selected)d.glow({x,y,w,h},19,9,c(LIME,.20f));
}
void photo(DrawList& d,unsigned id,float x,float y,float w,float h,unsigned fallback=0x202b30){
 if(id)d.image(id,{x,y,w,h},kFullUv,c(WHITE));
 else d.gradient_rect({x,y,w,h},0,c(fallback),c(0x0a0f14));
}
void tag(DrawList& d,const Font& f,unsigned a,const std::string& value,float x,float y,unsigned color=LIME){
 text(d,f,a,value,x,y,18,color);
}
std::string pretty_time(int ms){
 const int t=std::max(0,(ms+999)/1000);
 char s[24];snprintf(s,sizeof(s),"%02d:%02d",t/60,t%60);return s;
}
std::string minutes(int sec){return std::to_string((sec+59)/60)+" MIN";}
void header(DrawList& d,const Font& f,unsigned a,const State& s){
 d.gradient_rect({0,0,1920,1080},0,c(0x0a101a),c(0x05080f));
 d.rounded_rect({0,0,249,981},0,c(0x070b14,.99f));
 d.rounded_rect({0,982,1920,98},0,c(0x070a11));
 text(d,f,a,"FIT",43,91,52,WHITE);
 text(d,f,a,"BREW",133,91,52,LIME);
 text(d,f,a,"WORKOUTS  /  TRACK  /  PROGRESS",42,122,13,MUTED);
 text(d,f,a,"by YAHBALLON",44,161,15,0x8d9caf);
 const char* nav[]={"HOME","WORKOUTS","PROGRESS","PROGRAMS","SETTINGS"};
 const char* icon[]={"#","[]","|||","<>","*"};
 for(int i=0;i<5;i++){
  const int y=210+i*92;
  const bool active=(s.screen==Screen::Detail||s.screen==Screen::Session||s.screen==Screen::Completed)?i==1:i==s.nav;
  if(active)d.rounded_rect({22,float(y),214,69},12,c(LIME));
  text(d,f,a,icon[i],49,y+44,27,active?0x070a0d:0xa4b6cc);
  text(d,f,a,nav[i],91,y+43,22,active?0x06090a:0xd0daeb);
 }
 text(d,f,a,"FITBREW   v0.1.0",1698,83,23,WHITE);
 text(d,f,a,"100% FREE  -  OFFLINE",1698,111,15,LIME);
}
void footer(DrawList& d,const Font& f,unsigned a,const State& s){
 text(d,f,a,"X   SELECT / PAUSE",47,1040,21,0xbdd2e9);
 text(d,f,a,"O   BACK",318,1040,21,0xbdd2e9);
 text(d,f,a,"SQUARE   NEXT",523,1040,21,0xbdd2e9);
 text(d,f,a,"TRIANGLE   QUICK START / PREVIOUS",754,1040,20,0xbdd2e9);
 text(d,f,a,s.screen==Screen::Session?"L1  QUIT SESSION":"L1 / R1  SECTIONS",1847,1040,21,0xbdd2e9,Align::right);
}
void workout_tile(DrawList& d,const Font& f,unsigned a,unsigned img,
                  const Plan& p,float x,float y,float w,float h,bool chosen){
 panel(d,x,y,w,h,chosen);
 photo(d,img,x+5,y+5,w-10,h-10);
 d.gradient_rect({x+6,y+h*.5f,w-12,h*.48f},0,c(0x090d12,.1f),c(0x080c12,.98f));
 if(chosen)d.bordered_rect({x,y,w,h},17,c(0x0b1018,.08f),3.5f,c(LIME));
 text(d,f,a,p.title,x+18,y+h-67,23,WHITE);
 text(d,f,a,std::string(p.category)+"  /  "+p.level,x+18,y+h-40,15,0xd4e1ee);
 text(d,f,a,minutes(duration_seconds(p)),x+18,y+h-15,14,LIME);
}
void home(DrawList& d,const Font& f,unsigned a,const State& s,const std::array<unsigned,6>& images){
 // Left photo, live headline over a graded photograph (not the screenshot bitmap).
 photo(d,images[0],258,161,386,817);
 d.gradient_rect({258,161,386,450},0,c(0x070c14,.97f),c(0x070c14,.13f));
 text(d,f,a,"LEVEL UP",281,241,66);
 text(d,f,a,"YOUR",281,308,69);
 text(d,f,a,"BODY GOALS",281,373,51,LIME);
 tag(d,f,a,"BUILD  *  SWEAT  *  BE STRONGER",284,406,0xccd4df);
 text(d,f,a,"No gear. No fees.",286,949,21,WHITE);
 // Stat cards from actual local session history only.
 const std::string vals[]={std::to_string(s.history.completed),minutes(s.history.seconds),std::to_string(s.history.streak)};
 const char* labels[]={"WORKOUTS DONE","ACTIVE MINUTES","DAY STREAK"};
 for(int i=0;i<3;i++){
  float x=663+i*227.f;panel(d,x,160,213,164);
  text(d,f,a,i==0?"<>":i==1?"|||":"*",x+23,212,34,LIME);
  text(d,f,a,vals[i],x+23,264,34,WHITE);
  text(d,f,a,labels[i],x+23,301,15,MUTED);
 }
 panel(d,663,343,667,247);
 text(d,f,a,"WEEKLY ACTIVE MINUTES",686,389,29);
 long today=State::day_now();
 const char* days[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
 const int max_val=[&](){int m=60;for(int v:s.history.week)m=std::max(m,v);return m;}();
 for(int j=0;j<7;j++){
  int idx=int((today-6+j)%7);if(idx<0)idx+=7;
  const int val=s.history.week[idx];
  float x=696+j*90.f;float h=std::max(7.f,85.f*val/max_val);
  d.rounded_rect({x,535-h,55,h},8,c(j==6?LIME:0x4a5566));
  text(d,f,a,days[(int((today-6+j+4)%7)+7)%7],x+26,564,17,0xc6d1dd,Align::center);
 }
 text(d,f,a,"FEATURED WORKOUTS",663,642,30);
 for(int i=0;i<4;i++)
  workout_tile(d,f,a,images[plans[i].image],plans[i],663.f+i*169.f,665,154,280,i==s.focus);
 panel(d,1352,160,523,816);
 photo(d,images[4],1358,166,511,801);
 d.gradient_rect({1358,168,511,360},0,c(0x04070b,.8f),c(0x04070b,.05f));
 d.gradient_rect({1358,711,511,256},0,c(0x05070b,.05f),c(0x05070b,.98f));
 text(d,f,a,"TODAY'S WORKOUT",1380,206,22);
 text(d,f,a,plans[s.focus].title,1381,258,41);
 d.rounded_rect({1382,278,84,4},2,c(LIME));
 text(d,f,a,"NO EQUIPMENT",1381,732,23,LIME);
 text(d,f,a,minutes(duration_seconds(plans[s.focus])),1382,825,61,WHITE);
 text(d,f,a,"X  VIEW DETAILS   /   TRIANGLE  START",1382,905,20,WHITE);
}
void workouts(DrawList& d,const Font& f,unsigned a,const State& s,const std::array<unsigned,6>& images){
 text(d,f,a,"WORKOUT LIBRARY",287,209,63);
 text(d,f,a,"12 original bodyweight plans  /  36 exercises  /  all offline",290,245,23,MUTED);
 for(int i=0;i<PLAN_COUNT;i++){
  const int row=i/3,col=i%3;
  const float x=289+col*534.f,y=280+row*173.f;
  workout_tile(d,f,a,images[plans[i].image],plans[i],x,y,506,155,i==s.focus);
 }
}
void details(DrawList& d,const Font& f,unsigned a,const State& s,
             const std::array<unsigned,6>& images){
 const auto& p=plans[s.selected];
 photo(d,images[p.image],271,176,700,805);
 d.gradient_rect({271,176,700,345},0,c(0x070c13,.88f),c(0x070c13,.05f));
 text(d,f,a,p.title,311,265,59);
 text(d,f,a,p.subtitle,315,314,27,0xd4e4ef);
 tag(d,f,a,std::string(p.category)+"   *   "+p.level+"   *   NO EQUIPMENT",315,358);
 text(d,f,a,"X   START WORKOUT",315,916,34,LIME);
 panel(d,994,177,880,807);
 text(d,f,a,"WORKOUT BREAKDOWN",1034,250,44);
 text(d,f,a,minutes(duration_seconds(p))+"  /  "+std::to_string(p.count)+" MOVES  /  "+std::to_string(p.seconds)+"s ON + "+std::to_string(p.rest)+"s REST",1035,296,22,MUTED);
 for(int i=0;i<p.count;i++){
  const auto& e=exercises[p.sequence[i]];
  int col=i/5,row=i%5;
  float x=1036+col*410.f,y=349+row*118.f;
  photo(d,images[e.image],x,y,101,104);
  text(d,f,a,std::to_string(i+1)+". "+e.name,x+114,y+39,22);
  text(d,f,a,std::string(e.area)+"  /  "+std::to_string(p.seconds)+" SEC",x+114,y+73,16,LIME);
 }
 text(d,f,a,"LEFT / RIGHT  SWITCH PLAN",1050,934,19,0xaab9c8);
}
void session(DrawList& d,const Font& f,unsigned a,const State& s,const std::array<unsigned,6>& images){
 const auto& p=plans[s.selected];
 const auto& e=exercises[p.sequence[std::clamp(s.exercise_index,0,int(p.count)-1)]];
 photo(d,images[e.image],271,162,957,812);
 d.gradient_rect({271,704,957,266},0,c(0x070a0e,0),c(0x070a0e,.99f));
 text(d,f,a,p.title,300,224,40);
 text(d,f,a,s.resting?"RECOVERY / BREATHE":e.name,299,772,48,LIME);
 text(d,f,a,e.cue1,301,828,27);
 text(d,f,a,e.cue2,301,868,27);
 text(d,f,a,"EASIER: "+std::string(e.easier),301,908,21,0xe5ebf5);
 panel(d,1250,161,626,812);
 tag(d,f,a,"EXERCISE "+std::to_string(s.exercise_index+1)+" / "+std::to_string(p.count),1292,218);
 text(d,f,a,s.resting?"REST":"WORK",1290,275,43,LIME);
 text(d,f,a,pretty_time(s.remaining_ms),1290,407,122);
 float elapsed=(s.resting?float(p.rest):float(p.seconds))-s.remaining_ms/1000.f;
 float total=float(s.resting?p.rest:p.seconds);
 d.rounded_rect({1298,439,522,14},7,c(0x394148));
 d.rounded_rect({1298,439,522*std::clamp(elapsed/total,0.f,1.f),14},7,c(LIME));
 text(d,f,a,s.paused?"PAUSED":"KEEP MOVING",1290,527,29,s.paused?0xffcc70:WHITE);
 text(d,f,a,s.resting?"Get ready for the next exercise.":"Form first. Stop if you feel pain.",1290,577,23,0xb3c3d1);
 d.line(1292,634,1818,634,1,c(0x434a53));
 text(d,f,a,"NEXT",1291,690,20,MUTED);
 std::string next=s.exercise_index+1<p.count?exercises[p.sequence[s.exercise_index+1]].name:"WORKOUT COMPLETE";
 text(d,f,a,next,1291,729,28,WHITE);
 text(d,f,a,"X  PAUSE/RESUME",1292,810,24,LIME);
 text(d,f,a,"SQUARE  SKIP   /   TRIANGLE  PREVIOUS",1292,857,21,WHITE);
 text(d,f,a,"L1  EXIT WITHOUT SAVING",1292,909,20,0xcfd5de);
}
void progress(DrawList& d,const Font& f,unsigned a,const State& s){
 text(d,f,a,"YOUR PROGRESS",292,204,65);
 text(d,f,a,"Your activity stays on this console - no account or network needed.",295,243,24,MUTED);
 const char* labels[]={"WORKOUTS COMPLETED","ACTIVE TIME","CURRENT DAY STREAK"};
 std::string values[]={std::to_string(s.history.completed),minutes(s.history.seconds),std::to_string(s.history.streak)};
 for(int i=0;i<3;i++){
  float x=295+i*516.f;panel(d,x,297,484,223);
  tag(d,f,a,labels[i],x+30,354);
  text(d,f,a,values[i],x+31,456,72);
 }
 panel(d,294,553,1540,383);
 text(d,f,a,"LAST 7 UTC DAYS",324,609,34);
 int max_v=60;for(int v:s.history.week)max_v=std::max(v,max_v);
 long day=State::day_now();
 for(int i=0;i<7;i++){
  int idx=int((day-6+i)%7);if(idx<0)idx+=7;
  float x=385+i*197.f,h=std::max(4.f,170.f*s.history.week[idx]/max_v);
  d.rounded_rect({x,834-h,117,h},10,c(i==6?LIME:0x506278));
  text(d,f,a,std::to_string(s.history.week[idx]/60)+"m",x+58,866,23,WHITE,Align::center);
 }
 text(d,f,a,"X   OPEN WORKOUTS",350,917,19,LIME);
}
void programs(DrawList& d,const Font& f,unsigned a,const State& s,const std::array<unsigned,6>& images){
 text(d,f,a,"TRAINING PATHS",298,206,64);
 text(d,f,a,"Choose a path - each starts with one of our curated offline plans.",302,247,23,MUTED);
 const char* names[]={"START STRONG","BUILD ENDURANCE","MOVE & RECOVER"};
 const char* desc[]={"Gentle full-body strength","Intervals and controlled sweat","Low-impact mobility and posture"};
 const int plans_index[]={0,3,9};
 for(int i=0;i<3;i++){
  float x=299+i*521.f;
  panel(d,x,302,482,572,s.focus==i);
  photo(d,images[plans[plans_index[i]].image],x+5,307,472,422);
  d.gradient_rect({x+6,550,469,176},0,c(0x050b14,.05f),c(0x050b14,.98f));
  text(d,f,a,names[i],x+22,685,33);
  text(d,f,a,desc[i],x+22,728,19,MUTED);
  text(d,f,a,"X  CHOOSE PROGRAM",x+22,831,24,LIME);
 }
}
void settings(DrawList& d,const Font& f,unsigned a,const State& s){
 text(d,f,a,"SETTINGS",299,214,64);
 const std::string entries[]={"Preferred effort: "+std::string(s.level==0?"BEGINNER":s.level==1?"INTERMEDIATE":"ADVANCED"),
  "RESET LOCAL PROGRESS","ABOUT FITBREW"};
 for(int i=0;i<3;i++){
  float y=303+i*139.f;panel(d,299,y,1470,110,s.settings_focus==i);
  text(d,f,a,entries[i],332,y+70,29,s.settings_focus==i?LIME:WHITE);
 }
 text(d,f,a,"All sessions are bodyweight-only. No Bluetooth trackers or fake sensor data.",318,830,24,MUTED);
 text(d,f,a,"Before intense exercise, assess your health and start at a comfortable effort.",318,875,24,MUTED);
}
void done(DrawList& d,const Font& f,unsigned a,const State& s){
 panel(d,398,213,1323,694,true);
 text(d,f,a,"WORKOUT COMPLETE!",479,367,78,LIME);
 text(d,f,a,"Great work - your session has been saved locally.",481,431,30);
 text(d,f,a,plans[s.selected].title,483,526,35);
 text(d,f,a,minutes(duration_seconds(plans[s.selected]))+" ACTIVE TIME",483,611,49);
 text(d,f,a,"X  RETURN HOME     /     TRIANGLE  REPEAT",483,806,25,0xd6e3ef);
}
void exit_dialog(DrawList& d,const Font& f,unsigned a,const State& s){
 panel(d,635,326,880,458);
 text(d,f,a,"EXIT FITBREW?",705,410,61);
 text(d,f,a,"Your completed workouts are saved.",711,475,29,MUTED);
 const char* labels[]={"YES, EXIT","CANCEL"};
 for(int i=0;i<2;i++){
  bool chosen=i==s.focus;float x=705+i*353.f;
  d.rounded_rect({x,558,320,112},15,c(chosen?LIME:0x1c2633));
  text(d,f,a,labels[i],x+159,626,29,chosen?DARK:WHITE,Align::center);
 }
}
}
int main(){
 logs("BOOT");
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 const auto font_data=asset("fonts/inter-semibold.huifont",5*1024*1024);
 Font font;if(!font.load(font_data)){logs("FONT_FAIL");hui::sys::park();}
 unsigned atlas=renderer.batch().create_font_texture(font);
 std::array<unsigned,6> photos{};for(int i=0;i<6;i++)photos[i]=load_image(renderer,"pose_"+std::to_string(i)+".jpg");
 hui::ps5::Pad pad;pad.open();
 hui::InputTracker tracker;hui::InputSettings prefs;
 prefs.repeat_delay_us=330000;prefs.repeat_interval_us=135000;tracker.set_settings(prefs);
 State state;load_progress(state);
 uint64_t last=uint64_t(hui::sys::monotonic_us());
 for(;;){
  uint64_t now=uint64_t(hui::sys::monotonic_us());
  const int dt=int(std::min<uint64_t>(now-last,250000)/1000);last=now;
  std::array<hui::PadSample,64> samples{};
  const size_t count=pad.read(samples);
  const auto f=tracker.update(std::span<const hui::PadSample>(samples.data(),count),now);
  const Input input{
   .up=f.nav==hui::Direction::up,.down=f.nav==hui::Direction::down,
   .left=f.nav==hui::Direction::left,.right=f.nav==hui::Direction::right,
   .confirm=f.is_pressed(hui::Action::confirm),.back=f.is_pressed(hui::Action::back),
   .triangle=f.is_pressed(hui::Action::north),.square=f.is_pressed(hui::Action::west),
   .options=f.is_pressed(hui::Action::menu),
   .l1=f.is_pressed(hui::Action::page_prev),.r1=f.is_pressed(hui::Action::page_next)
  };
  const auto old_screen=state.screen;
  if(state.handle(input)){
    if(state.dirty)save_progress(state);
    logs("EXIT");pad.close();for(auto& id:photos)if(id)glDeleteTextures(1,&id);
    renderer.release();display.close();hui::sys::quit();
  }
  state.tick(dt);
  if(old_screen!=state.screen){
   if(state.screen==Screen::Completed){logs("WORKOUT_COMPLETED");pad.rumble(.28f,.16f);}
   else if(state.screen==Screen::Session)logs("WORKOUT_STARTED");
  }
  if(state.dirty){save_progress(state);state.dirty=false;}
  DrawList ui;header(ui,font,atlas,state);
  switch(state.screen){
   case Screen::Home:home(ui,font,atlas,state,photos);break;
   case Screen::Workouts:workouts(ui,font,atlas,state,photos);break;
   case Screen::Progress:progress(ui,font,atlas,state);break;
   case Screen::Programs:programs(ui,font,atlas,state,photos);break;
   case Screen::Settings:settings(ui,font,atlas,state);break;
   case Screen::Detail:details(ui,font,atlas,state,photos);break;
   case Screen::Session:session(ui,font,atlas,state,photos);break;
   case Screen::Completed:done(ui,font,atlas,state);break;
   case Screen::Exit:exit_dialog(ui,font,atlas,state);break;
  }
  footer(ui,font,atlas,state);
  renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();pad.tick(float(dt)/1000.f);hui::sys::sleep_us(16000);
 }
}
