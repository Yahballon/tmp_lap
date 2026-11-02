// SPDX-License-Identifier: MIT
// STANDBY CLOCK by YAHBALLON v0.1.1 — native PS5 renderer.
#include "clock_logic.hpp"
#include "weather_client.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <sys/stat.h>
using namespace hui::gfx;
using namespace standby;
namespace {
constexpr int TITLE=99876;
constexpr float PI=3.14159265358979323846f;
Color color(unsigned hex,float alpha=1.f){return Color::rgb(hex,alpha);}
void logline(const char* msg){
 mkdir("/data/StandbyClock",0755);
 if(FILE* f=fopen("/data/StandbyClock/clock.log","ab")){
  fprintf(f,"[STANDBY CLOCK v0.1.1] %s\n",msg);fclose(f);
 }
}
std::string readfile(const std::string& path,size_t maxBytes=5*1024*1024){
 FILE* f=fopen(path.c_str(),"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 long len=ftell(f);if(len<=0||size_t(len)>maxBytes){fclose(f);return {};}
 rewind(f);std::string s(size_t(len),'\0');
 if(fread(s.data(),1,s.size(),f)!=s.size())s.clear();
 fclose(f);return s;
}
std::string asset(const char* suffix){
 for(const char* prefix:{"/app0/assets/","/data/homebrew/PPSA99876/assets/","assets/"}){
  auto bytes=readfile(std::string(prefix)+suffix);
  if(!bytes.empty())return bytes;
 }
 return {};
}
void save(const Settings& s){
 mkdir("/data/StandbyClock",0755);
 FILE* f=fopen("/data/StandbyClock/settings.tmp","wb");
 if(!f){logline("SAVE_OPEN_FAILED");return;}
 fprintf(f,"version=2\ntheme=%d\nbrightness=%d\nuse24h=%d\nalarmOn=%d\nalarmHour=%d\nalarmMinute=%d\nutcOffset=%d\npixelDrift=%d\noledGuard=%d\nautoDim=%d\nweatherEnabled=%d\n",
 s.theme,s.brightness,s.use24h,s.alarmOn,s.alarmHour,s.alarmMinute,s.utcOffset,s.pixelDrift,
 s.oledGuard,s.autoDim,s.weatherEnabled);
 if(fclose(f)!=0||rename("/data/StandbyClock/settings.tmp","/data/StandbyClock/settings.ini")!=0)
  logline("SAVE_FAILED");
}
void load(Settings& s){
 auto bytes=readfile("/data/StandbyClock/settings.ini",2048);if(bytes.empty())return;
 Settings temp;int version=0;
 int count=sscanf(bytes.c_str(),
  "version=%d\ntheme=%d\nbrightness=%d\nuse24h=%d\nalarmOn=%d\nalarmHour=%d\nalarmMinute=%d\nutcOffset=%d\npixelDrift=%d\noledGuard=%d\nautoDim=%d\nweatherEnabled=%d",
  &version,&temp.theme,&temp.brightness,&temp.use24h,&temp.alarmOn,&temp.alarmHour,
  &temp.alarmMinute,&temp.utcOffset,&temp.pixelDrift,&temp.oledGuard,&temp.autoDim,&temp.weatherEnabled);
 // Preserve existing v0.1.0 preferences and enable new optional features.
 if((version==1&&count>=9)||(version==2&&count==12)){
  validate(temp);s=temp;
 }else logline("INVALID_SETTINGS_DEFAULTS");
}
unsigned palette(int theme){
 static constexpr unsigned colors[]={0xff141b,0xffae20,0x5acaff,0xf2f5ff};
 return colors[std::clamp(theme,0,3)];
}
const char* theme_name(int theme){
 static constexpr const char* names[]={"RED","AMBER","ICE","WHITE"};
 return names[std::clamp(theme,0,3)];
}
void text(DrawList& d,const Font& f,unsigned a,const std::string& val,float x,float y,float size,unsigned tint,Align align=Align::left){
 d.text(f,a,val,x,y,size,color(tint),align);
}
void stroke(DrawList& d,float x,float y,float w,float h,float thick,unsigned c){
 d.rounded_rect({x,y,w,h},thick*.5f,color(c));
}
void analog(DrawList& d,const Font& f,unsigned a,const ClockTime& t,unsigned accent){
 float cx=1608.f,cy=490.f,R=176.f;
 for(int i=0;i<60;i++){
  float angle=float(i)*PI/30.f-PI*.5f;
  float outer=R,inner=R-(i%5==0?19.f:9.f);
  float co=std::cos(angle),si=std::sin(angle);
  d.line(cx+inner*co,cy+inner*si,cx+outer*co,cy+outer*si,i%5==0?5.f:2.f,color(accent));
 }
 for(int n=1;n<=12;n+=3){
  float theta=float(n)*PI/6.f-PI/2.f;
  text(d,f,a,std::to_string(n),cx+134*std::cos(theta),cy+134*std::sin(theta)+13,36,accent,Align::center);
 }
 auto hand=[&](float angle,float length,float width){
  const float phi=angle-PI/2.f;
  d.line(cx,cy,cx+length*std::cos(phi),cy+length*std::sin(phi),width,color(accent));
 };
 hand((t.hour%12+t.minute/60.f)*PI/6.f,100,13);
 hand((t.minute+t.second/60.f)*PI/30.f,139,10);
 hand(t.second*PI/30.f,161,2);
 d.circle(cx,cy,10,color(0x000000));
 d.ring(cx,cy,10,5,color(accent));
}
void big_time(DrawList& d,const Font& f,unsigned a,const ClockTime& t,const Settings& s,unsigned accent){
 // Vector/bitmap font renders each character natively. No static image or screenshot is displayed.
 const std::string str=digital(t,s.use24h);
 const float x[5]={80.f,327.f,586.f,704.f,955.f};
 const float sizes[5]={386,386,344,386,386};
 for(int i=0;i<5;i++){
  char c[2]={str[size_t(i)],0};
  if(i==2){
   d.circle(614,350,31,color(accent));d.circle(614,548,31,color(accent));
  }else text(d,f,a,c,x[i],618,sizes[i],accent);
 }
}
void calendar(DrawList& d,const Font& f,unsigned a,const ClockTime& t,unsigned accent){
 static const char* names[]={"M","T","W","T","F","S","S"};
 const auto dates=week_dates(t);
 int selected=(t.weekday+6)%7;
 for(int i=0;i<7;i++){
  const float x=1321+i*81.f;
  text(d,f,a,names[i],x,874,27,accent,Align::center);
  if(i==selected)d.circle(x,931,31,color(accent));
  text(d,f,a,std::to_string(dates[i]),x,942,27,i==selected?0x000000:accent,Align::center);
 }
}
void alarm_symbol(DrawList& d,float x,float y,unsigned accent){
 d.ring(x,y,25,4,color(accent));
 d.line(x,y,x,y-14,4,color(accent));
 d.line(x,y,x+12,y+10,4,color(accent));
 d.line(x-20,y-34,x-36,y-19,5,color(accent));
 d.line(x+20,y-34,x+36,y-19,5,color(accent));
 d.line(x-18,y+21,x-27,y+32,4,color(accent));
 d.line(x+18,y+21,x+27,y+32,4,color(accent));
}
void home(DrawList& d,const Font& f,unsigned a,const ClockTime& t,const Settings& s,const weather::Data& w,bool alert){
 unsigned accent=palette(s.theme);
 d.rounded_rect({0,0,1920,1080},0,color(0x000000));
 // Entire content subtly migrates to prevent permanent fixed-position highlights on OLED screens.
 d.push_clip({0,0,1920,1080});
 // Header remains minimal to preserve the visual reference.
 text(d,f,a,"STANDBY CLOCK by YAHBALLON",70,68,30,accent);
 text(d,f,a,"v0.1.1",1847,66,24,accent,Align::right);
 d.line(66,94,1853,94,2,color(accent));
 big_time(d,f,a,t,s,accent);
 d.line(1336,140,1336,704,2,color(accent));
 text(d,f,a,weekday_name(t.weekday),1437,204,72,accent);
 char date[40];snprintf(date,sizeof(date),"%s %02d",month_name(t.month),t.day);
 text(d,f,a,date,1438,270,42,accent);
 // Keep AM/PM away from the giant numerals and bottom of the analog clock.
 if(!s.use24h)text(d,f,a,t.hour<12?"AM":"PM",1830,272,24,accent,Align::right);
 analog(d,f,a,t,accent);
 d.line(80,766,1835,766,2,color(accent));
 alarm_symbol(d,134,864,accent);
 text(d,f,a,"ALARM",228,833,25,accent);
 text(d,f,a,s.alarmOn?alarm_text(s):"OFF",229,921,55,accent);
 d.line(540,804,540,948,2,color(accent));
 // Live weather replaces the old Homebrew READY panel. Cloud mark is native geometry.
 d.circle(642,868,22,color(accent));
 d.circle(668,856,28,color(accent));
 d.circle(694,871,20,color(accent));
 d.rounded_rect({620,868,95,23},10,color(accent));
 text(d,f,a,"WEATHER",739,839,32,accent);
 if(!s.weatherEnabled){
  text(d,f,a,"DISABLED",739,909,22,accent);
 }else if(w.valid){
  text(d,f,a,std::to_string(w.temperature)+" C",1176,839,36,accent,Align::right);
  std::string locality=w.city;
  if(locality.size()>18)locality=locality.substr(0,18);
  std::string detail=locality+" / "+w.condition;
  if(detail.size()>40)detail.resize(40);
  text(d,f,a,detail,739,902,21,accent);
  if(w.stale)text(d,f,a,"CACHED",1174,935,17,accent,Align::right);
 }else{
  text(d,f,a,w.busy?"LOCATING / LOADING":"WEATHER OFFLINE",739,909,22,accent);
 }
 d.line(1214,804,1214,948,2,color(accent));
 calendar(d,f,a,t,accent);
 text(d,f,a,"X SETTINGS     SQUARE ALARM     TRIANGLE THEME     L1/R1 BRIGHTNESS     O EXIT",960,1046,21,accent,Align::center);
 d.pop_clip();
 if(alert){
  d.bordered_rect({330,348,1200,295},27,color(0x190000,.96f),5,color(accent));
  text(d,f,a,"ALARM",930,467,85,accent,Align::center);
  text(d,f,a,"X DISMISS     O SNOOZE 5 MIN",930,551,31,accent,Align::center);
 }
}
const char* setting_label(int index){
 static const char* labels[]={"THEME","BRIGHTNESS","TIME FORMAT","ALARM ENABLED",
   "ALARM HOUR","ALARM MINUTE","TIMEZONE","OLED PIXEL SHIFT","OLED PROTECTION",
   "AUTO DIM / NIGHT","AUTO WEATHER","CLOSE SETTINGS"};
 return labels[std::clamp(index,0,11)];
}
std::string setting_value(int index,const Settings& s){
 switch(index){
 case 0:return theme_name(s.theme);
 case 1:return std::to_string(s.brightness)+" / 5";
 case 2:return s.use24h?"24 HOURS":"12 HOURS";
 case 3:return s.alarmOn?"ON":"OFF";
 case 4:{char b[8];snprintf(b,sizeof(b),"%02d",s.alarmHour);return b;}
 case 5:{char b[8];snprintf(b,sizeof(b),"%02d",s.alarmMinute);return b;}
 case 6:return s.utcOffset==99?"SYSTEM LOCAL":"UTC "+std::string(s.utcOffset>=0?"+":"")+std::to_string(s.utcOffset);
 case 7:return s.pixelDrift?"ON":"OFF";
 case 8:return s.oledGuard?"ON":"OFF";
 case 9:return s.autoDim?"ON":"OFF";
 case 10:return s.weatherEnabled?"ON":"OFF";
 default:return "X CONFIRM";
 }
}
void settings_screen(DrawList& d,const Font& f,unsigned a,const Settings& s,int focus){
 unsigned accent=palette(s.theme);
 d.rounded_rect({0,0,1920,1080},0,color(0x000000));
 text(d,f,a,"STANDBY CLOCK by YAHBALLON",90,87,36,accent);
 text(d,f,a,"SETTINGS",93,179,69,accent);
 text(d,f,a,"v0.1.1",1810,88,25,accent,Align::right);
 d.line(89,211,1810,211,2,color(accent));
 for(int i=0;i<12;i++){
  float y=238+i*61.f;
  if(focus==i)d.bordered_rect({98,y-8,1714,57},11,color(0x1e1212),3,color(accent));
  text(d,f,a,setting_label(i),125,y+31,24,accent);
  text(d,f,a,setting_value(i,s),1760,y+31,24,accent,Align::right);
 }
 text(d,f,a,"D-PAD SELECT / ADJUST     X CHANGE     O SAVE AND RETURN",960,1036,24,accent,Align::center);
}
void modify(Settings& s,int index,int sign){
 switch(index){
 case 0:s.theme=(s.theme+4+sign)%4;break;
 case 1:s.brightness=std::clamp(s.brightness+sign,1,5);break;
 case 2:s.use24h=!s.use24h;break;
 case 3:s.alarmOn=!s.alarmOn;break;
 case 4:s.alarmHour=(s.alarmHour+24+sign)%24;break;
 case 5:s.alarmMinute=(s.alarmMinute+60+sign)%60;break;
 case 6:if(s.utcOffset==99)s.utcOffset=0;
        else if(sign>0){s.utcOffset=s.utcOffset>=14?99:s.utcOffset+1;}
        else{s.utcOffset=s.utcOffset<=-12?99:s.utcOffset-1;}break;
 case 7:s.pixelDrift=!s.pixelDrift;break;
 case 8:s.oledGuard=!s.oledGuard;break;
 case 9:s.autoDim=!s.autoDim;break;
 case 10:s.weatherEnabled=!s.weatherEnabled;break;
 }
 validate(s);
}
} // namespace
int main(){
 logline("BOOT");
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;if(!font.load(asset("fonts/inter-semibold.huifont"))){logline("FONT_MISSING");hui::sys::park();}
 unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 hui::InputTracker tracker;
 hui::InputSettings ip;ip.repeat_delay_us=330000;ip.repeat_interval_us=115000;tracker.set_settings(ip);
 Settings s;load(s);
 weather::init();
 if(s.weatherEnabled)weather::refresh();
 bool settingsOpen=false,alert=false;
 int focus=0,alertDay=-1;
 std::time_t snoozeUntil=0;
 bool priorDue=false;
 uint64_t lastLog=0;
 const uint64_t bootTick=uint64_t(hui::sys::monotonic_us());
 uint64_t lastInputTick=bootTick,lastWeatherTick=bootTick;
 for(;;){
  const uint64_t tick=uint64_t(hui::sys::monotonic_us());
  std::array<hui::PadSample,64> samples{};
  const size_t n=pad.read(samples);
  const auto frame=tracker.update(std::span<const hui::PadSample>(samples.data(),n),tick);
  auto press=[&](hui::Action a){return frame.is_pressed(a);};
  if(frame.pressed!=0||frame.nav!=hui::Direction::none)lastInputTick=tick;
  if(s.weatherEnabled && tick-lastWeatherTick>=900000000ULL){
   if(weather::refresh())lastWeatherTick=tick;
  }
  const std::time_t epoch=std::time(nullptr);
  const ClockTime t=from_timestamp(epoch,s.utcOffset);
  const bool due=alarm_due(s,t);
  if(due&&!priorDue && alertDay!=day_key(t)){
   alert=true;alertDay=day_key(t);logline("ALARM_TRIGGER");
  }
  if(snoozeUntil>0 && epoch>=snoozeUntil){snoozeUntil=0;alert=true;logline("SNOOZE_TRIGGER");}
  priorDue=due;
  if(alert){
   if(press(hui::Action::confirm)){alert=false;snoozeUntil=0;logline("ALARM_DISMISS");}
   if(press(hui::Action::back)){alert=false;snoozeUntil=epoch+300;logline("ALARM_SNOOZE");}
  }else if(settingsOpen){
   if(frame.nav==hui::Direction::up)focus=(focus+11)%12;
   if(frame.nav==hui::Direction::down)focus=(focus+1)%12;
   if(frame.nav==hui::Direction::left)modify(s,focus,-1);
   if(frame.nav==hui::Direction::right)modify(s,focus,1);
   if(press(hui::Action::confirm)){
    if(focus==11){settingsOpen=false;save(s);if(s.weatherEnabled)weather::refresh();}
    else modify(s,focus,1);
   }
   if(press(hui::Action::back)||press(hui::Action::menu)){settingsOpen=false;save(s);if(s.weatherEnabled)weather::refresh();}
  }else{
   if(press(hui::Action::confirm)||press(hui::Action::menu)){focus=0;settingsOpen=true;}
   if(press(hui::Action::west)){focus=3;settingsOpen=true;}
   if(press(hui::Action::north)){s.theme=(s.theme+1)%4;save(s);}
   if(press(hui::Action::page_prev)){s.brightness=std::max(1,s.brightness-1);save(s);}
   if(press(hui::Action::page_next)){s.brightness=std::min(5,s.brightness+1);save(s);}
   if(press(hui::Action::back)){
    save(s);logline("EXIT");pad.close();renderer.release();display.close();hui::sys::quit();
   }
  }
  DrawList d;
  const uint64_t secondsSinceInput=(tick-lastInputTick)/1000000ULL;
  // Entire UI shifts as one unit, including analogue dial and footer.
  float shiftX=0,shiftY=0;
  if(s.oledGuard&&s.pixelDrift&&!settingsOpen&&!alert){
   const int phase=int((tick-bootTick)/90000000ULL%8ULL);
   static constexpr float xs[]={-10,6,12,-4,8,-12,4,0};
   static constexpr float ys[]={-7,8,-2,10,3,-9,5,0};
   shiftX=xs[phase];shiftY=ys[phase];
  }
  // 20 seconds of black display per 30 minutes, unless user is interacting
  // or alarm is visible. Black rest never puts the console into standby.
  const bool oledRest=s.oledGuard&&!settingsOpen&&!alert&&secondsSinceInput>90 &&
     ((tick-bootTick)/1000000ULL%1800ULL)>=1780ULL;
  if(oledRest)d.rounded_rect({0,0,1920,1080},0,color(0x000000));
  else if(settingsOpen)settings_screen(d,font,atlas,s,focus);
  else{
   d.rounded_rect({0,0,1920,1080},0,color(0x000000));
   d.push_transform(1.f,0,0,shiftX,shiftY);
   home(d,font,atlas,t,s,weather::snapshot(),alert);
   d.pop_transform();
  }
  // OLED guard is an in-app safeguard, not the TV's hardware pixel refresh.
  float dim=float(5-s.brightness)*0.16f;
  if(s.oledGuard&&s.autoDim&&!settingsOpen&&!alert){
   if(secondsSinceInput>=600)dim+=.36f;
   else if(secondsSinceInput>=120)dim+=.19f;
   if(t.hour>=23||t.hour<7)dim+=.14f;
  }
  dim=std::clamp(dim,0.f,.83f);
  if(dim>0)d.rounded_rect({0,0,1920,1080},0,color(0x000000,dim));
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();
  if(tick-lastLog>60000000ULL){lastLog=tick;logline("CLOCK_TICK");}
  pad.tick(.016f);hui::sys::sleep_us(16000);
 }
}
