// P2P PONG v0.1.0 by YAHBALLON — relay-assisted multiplayer prototype.
#include "core/pong.hpp"
#include "network.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
using namespace hui::gfx;
namespace {
using hui::gfx::Color;
const Color white=Color::rgb(0xeaf7ff),muted=Color::rgb(0x91afce),cyan=Color::rgb(0x4ceaff),pink=Color::rgb(0xff508b);
std::string readfile(const char* path){FILE*f=fopen(path,"rb");if(!f)return {};std::string s;char b[1024];size_t n;while((n=fread(b,1,sizeof(b),f))>0&&s.size()<2000000)s.append(b,n);fclose(f);return s;}
void log(const std::string& s){mkdir("/data/P2PPong",0755);mkdir("/data/P2PPong/logs",0755);if(auto*f=fopen("/data/P2PPong/logs/pong.log","a")){fprintf(f,"%s\n",s.c_str());fclose(f);}}
void text(DrawList& d,const Font& f,unsigned a,const std::string& t,float x,float y,float size,Color c=white,Align align=Align::left){d.text(f,a,t,x,y,size,c,align);}
void card(DrawList& d,float x,float y,float w,float h,bool on=false){d.bordered_rect({x,y,w,h},14,Color::rgb(on?0x124c6e:0x101c32),on?3.f:1.3f,on?cyan:Color::rgb(0x2b436a));}
std::string safe(std::string s){std::string v;for(char c:s)if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==' '||c=='_'||c=='-')v+=c;return v.substr(0,24);}
std::string config(const std::string& s,const std::string& key,const std::string& fallback){auto p=s.find(key+"=");if(p==std::string::npos)return fallback;p+=key.size()+1;return s.substr(p,s.find_first_of("\r\n",p)-p);}
void save(const std::string& name,const std::string& room){mkdir("/data/P2PPong",0755);if(auto*f=fopen("/data/P2PPong/settings.ini","w")){fprintf(f,"name=%s\nroom=%s\n",name.c_str(),room.c_str());fclose(f);}}
struct Room{std::string id,title,host;};
}
int main(){
 log("BOOT v0.1.0");
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;
 std::string bytes=readfile("/app0/assets/fonts/inter-semibold.huifont");
 if(bytes.empty())bytes=readfile("/data/homebrew/PPSA99873/assets/fonts/inter-semibold.huifont");
 if(!font.load(bytes)){log("FONT_FAIL");hui::sys::park();}
 unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();pongnet::Client net;
 const auto cfg=readfile("/data/P2PPong/network.conf");
 const auto settings=readfile("/data/P2PPong/settings.ini");
 std::string host=config(cfg,"relay","127.0.0.1");
 int port=std::atoi(config(cfg,"port","39733").c_str());
 std::string name=safe(config(settings,"name","PS5PLAYER"));
 std::string title=safe(config(settings,"room","MY PONG ROOM"));
 std::string info="Set /data/P2PPong/network.conf relay=<PUBLIC IP> first";
 std::string code;std::vector<Room> rooms;
 bool lobby=false,joined=false,playing=false,is_host=false;
 int screen=0,focus=0,selected=0;float remote=0.f;float elapsed=0.f,sendtimer=0.f;
 pong::State game;
 const std::string alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-";
 size_t editindex=0;int editfield=0;
 unsigned prev=0;
 for(;;){
  std::array<hui::PadSample,64> samples{};unsigned pressed=0;size_t n=pad.read(samples);
  for(size_t i=0;i<n;i++){pressed|=samples[i].buttons&~prev;prev=samples[i].buttons;}
  bool up=pressed&hui::pad_bits::kUp,down=pressed&hui::pad_bits::kDown;
  bool left=pressed&hui::pad_bits::kLeft,right=pressed&hui::pad_bits::kRight;
  bool x=pressed&hui::pad_bits::kCross,o=pressed&hui::pad_bits::kCircle;
  for(const auto& line:net.poll()){
   if(line=="LIST_BEGIN")rooms.clear();
   else if(line.rfind("ROOM|",0)==0){auto a=line.find('|',5),b=a==std::string::npos?a:line.find('|',a+1);if(b!=std::string::npos)rooms.push_back({line.substr(5,a-5),line.substr(a+1,b-a-1),line.substr(b+1)});}
   else if(line.rfind("CREATED|",0)==0){code=line.substr(8);lobby=true;is_host=true;joined=true;playing=false;info="Lobby created: "+code;}
   else if(line.rfind("JOINED|",0)==0){code=line.substr(7,6);lobby=true;joined=true;is_host=false;playing=false;info="Joined "+code+", waiting for host";}
   else if(line.rfind("PEER_JOINED|",0)==0){info="Opponent: "+line.substr(12)+" | X Start";}
   else if(line=="STARTED"){playing=true;screen=3;game=pong::State{};info="Match started";}
   else if(line=="PEER_LEFT"){lobby=false;playing=false;joined=false;screen=1;info="Peer disconnected";}
   else if(line.rfind("INPUT|",0)==0){if(is_host)remote=std::clamp((float)std::atof(line.c_str()+6),-1.f,1.f);}
   else if(line.rfind("STATE|",0)==0&&!is_host){
    pong::State s;
    if(std::sscanf(line.c_str()+6,"%f,%f,%f,%f,%f,%f,%d,%d",&s.left,&s.right,&s.x,&s.y,&s.vx,&s.vy,&s.score_l,&s.score_r)==8)game=s;
   }
   else if(line.rfind("ERROR|",0)==0)info=line;
  }
  if(!net.online()&&joined){joined=lobby=playing=false;screen=0;info="Relay disconnected";}
  if(screen==0){
   if(up&&focus>0)--focus;if(down&&focus<4)++focus;
   if(x){
    if(focus==0){if(net.connect_to(host,port)){net.send("NAME|"+name);screen=1;net.send("LIST");info="Connected to relay";}else info=net.status;}
    if(focus==1){screen=4;editfield=0;editindex=0;}
    if(focus==2){screen=4;editfield=1;editindex=0;}
    if(focus==3){if(net.connect_to(host,port)){net.send("NAME|"+name);net.send("CREATE|"+title);screen=2;}else info=net.status;}
    if(focus==4)break;
   }
  }else if(screen==4){
   std::string& field=editfield==0?name:title;
   if(left&&editindex>0)--editindex;if(right&&editindex+1<alphabet.size())++editindex;
   if(up&&field.size()<24)field+=alphabet[editindex];
   if(down&&!field.empty())field.pop_back();
   if(x){if(!field.empty()){save(name,title);screen=0;info="Saved profile";}}
   if(o)screen=0;
  }else if(screen==1){
   if(up&&selected>0)--selected;if(down&&selected+1<(int)rooms.size())++selected;
   if(pressed&hui::pad_bits::kTriangle)net.send("LIST");
   if(x&&!rooms.empty()){net.send("JOIN|"+rooms[selected].id);screen=2;}
   if(pressed&hui::pad_bits::kSquare){net.send("CREATE|"+title);screen=2;}
   if(o){net.close();screen=0;}
  }else if(screen==2){
   if(x&&is_host)net.send("START");
   if(o){net.send("LEAVE");net.close();joined=lobby=false;screen=0;}
  }else if(screen==3){
   float move=up?-1.f:down?1.f:0.f;
   for(size_t i=0;i<n;i++){auto b=samples[i].buttons;if(b&hui::pad_bits::kUp)move=-1;else if(b&hui::pad_bits::kDown)move=1;}
   if(is_host)pong::step(game,1.f/60.f,move,remote);
   else{if(sendtimer>=.05f){net.send("INPUT|"+std::to_string(int(move)));sendtimer=0;}}
   sendtimer+=1.f/60.f;
   if(is_host&&sendtimer>.033f){char b[256];snprintf(b,sizeof(b),"STATE|%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%d",game.left,game.right,game.x,game.y,game.vx,game.vy,game.score_l,game.score_r);net.send(b);sendtimer=0;}
   if(o){net.send("LEAVE");net.close();playing=joined=lobby=false;screen=0;}
  }
  DrawList d;d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x050a1a),Color::rgb(0x0d1632));
  text(d,font,atlas,"P2P PONG",70,126,87,cyan);text(d,font,atlas,"by YAHBALLON   v0.1.0  /  RELAY-ASSISTED P2P TEST",75,169,21,muted);
  if(screen==0){
   const char* items[]={"CONNECT / FIND LOBBIES","CHANGE PLAYER NAME","CHANGE LOBBY NAME","CREATE LOBBY","EXIT"};
   for(int i=0;i<5;i++){card(d,75,235+i*112,900,95,focus==i);text(d,font,atlas,items[i],108,294+i*112,31,focus==i?cyan:white);}
   text(d,font,atlas,"PLAYER: "+name,1100,280,28);
   text(d,font,atlas,"LOBBY: "+title,1100,347,26,muted);
   text(d,font,atlas,"RELAY: "+host+":"+std::to_string(port),1100,415,22,muted);
  }else if(screen==4){
   text(d,font,atlas,editfield==0?"EDIT PLAYER NAME":"EDIT LOBBY NAME",95,269,36);
   card(d,90,340,1680,120);text(d,font,atlas,editfield==0?name:title,125,416,45,cyan);
   text(d,font,atlas,std::string("CHARACTER: [")+alphabet[editindex]+"]",125,520,34,pink);
   text(d,font,atlas,"LEFT/RIGHT choose letter  UP append  DOWN delete  X save",125,605,26);
  }else if(screen==1){
   text(d,font,atlas,"PUBLIC LOBBIES  |  TRIANGLE REFRESH  |  SQUARE CREATE",95,250,28,cyan);
   if(rooms.empty())text(d,font,atlas,"No open rooms found. Create one with SQUARE.",130,402,28,muted);
   for(size_t i=0;i<rooms.size()&&i<6;i++){card(d,90,310+109*i,1650,91,int(i)==selected);text(d,font,atlas,rooms[i].title+"    /    "+rooms[i].host+"    #"+rooms[i].id,127,368+109*i,28);}
  }else if(screen==2){
   text(d,font,atlas,"LOBBY "+code,95,330,62,cyan);
   text(d,font,atlas,is_host?"Waiting for second player / X START":"Waiting for host to start...",95,416,37);
  }else{
   text(d,font,atlas,std::to_string(game.score_l)+"    :    "+std::to_string(game.score_r),955,290,72,white,Align::center);
   d.rounded_rect({285,350,1350,550},14,Color::rgb(0x0a1833));
   for(int i=0;i<12;i++)d.rounded_rect({952.f,362.f+i*45,6,23},3,Color::rgb(0x4a657d));
   d.rounded_rect({320,350+game.left*550-59,21,118},8,cyan);
   d.rounded_rect({1570,350+game.right*550-59,21,118},8,pink);
   d.circle(285+game.x*1350,350+game.y*550,16,white);
   text(d,font,atlas,is_host?"HOST - LEFT PADDLE":"GUEST - RIGHT PADDLE",955,956,27,cyan,Align::center);
  }
  card(d,73,1000,1760,55);text(d,font,atlas,info.substr(0,110),97,1036,21,muted);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
 net.close();pad.close();renderer.release();display.close();hui::sys::quit();
}
