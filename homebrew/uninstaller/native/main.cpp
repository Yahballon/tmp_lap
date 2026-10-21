// SPDX-License-Identifier: GPL-3.0-or-later
#include "inventory.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include <array>
#include <cstdio>
#include <ctime>
#include <string>
using namespace hui::gfx;
namespace {
Color color(unsigned v,float a=1){return Color::rgb(v,a);}
void txt(DrawList& d,const Font& f,unsigned t,const std::string& s,float x,float y,int sz=21,Color c=color(0xf1f8ff),Align a=Align::left){d.text(f,t,s,x,y,sz,c,a);}
void panel(DrawList& d,float x,float y,float w,float h,bool active=false){d.bordered_rect({x,y,w,h},15,color(active?0x072856:0x061524),active?3:1.5,color(active?0x36c6ff:0x24547c));}
std::string read(const char* p){FILE* f=fopen(p,"rb");if(!f)return {};std::string s;char b[4096];size_t n;while((n=fread(b,1,sizeof b,f))>0&&s.size()<500000)s.append(b,n);fclose(f);return s;}
void trash(DrawList& d,float x,float y,float s){d.rounded_rect({x-5*s,y,65*s,12*s},4*s,color(0xff294d));d.bordered_rect({x,y+14*s,54*s,60*s},8*s,color(0x5b091e),3*s,color(0xff3d63));for(int i=0;i<3;i++)d.rounded_rect({x+(13+13*i)*s,y+25*s,5*s,36*s},2*s,color(0xff6d85));}
bool visible(const uninstaller::Item& a,int tab){return tab==0||(tab==1&&a.kind==uninstaller::Kind::Installed)||(tab==2&&a.kind==uninstaller::Kind::Homebrew)||(tab==3&&a.kind==uninstaller::Kind::UsbPackage);}
int count(const std::vector<uninstaller::Item>& v,int tab){int n=0;for(const auto& a:v)if(visible(a,tab))++n;return n;}
const uninstaller::Item* nth(const std::vector<uninstaller::Item>& v,int tab,int n){for(const auto& a:v)if(visible(a,tab)&&n--==0)return &a;return nullptr;}
void log(const std::string& s){mkdir("/data/uninstaller",0755);if(FILE* f=fopen("/data/uninstaller/uninstaller.log","ab")){fprintf(f,"%s\n",s.c_str());fclose(f);}}
void render(DrawList& d,const Font& f,unsigned a,const std::vector<uninstaller::Item>& items,int tab,int selected,int offset,bool confirm,const std::string& msg,const uninstaller::Storage& disk){
 const Color white=color(0xf1f8ff),muted=color(0x9eb7d7);
 d.gradient_rect({0,0,1920,1080},0,color(0x061a31),color(0x020512));
 d.gradient_rect({1250,0,670,1050},0,color(0x051b3c,.55f),color(0x210515,.55f));
 trash(d,70,34,.77f);txt(d,f,a,"Uninstaller",168,85,55);
 txt(d,f,a,"Remove games and homebrew apps from your console.",168,123,21,muted);
 txt(d,f,a,"Clean Storage.  More Space.  A Smoother Console.",1190,83,19,muted);
 time_t now=time(nullptr);tm* t=localtime(&now);char b[20]="--:--";if(t)strftime(b,sizeof b,"%H:%M",t);
 txt(d,f,a,b,1880,82,29,white,Align::right);
 panel(d,30,145,682,863);panel(d,723,145,615,863);panel(d,1349,145,541,863);
 const char* cats[]={"All","Games","Homebrew","USB PKGs"};
 for(int i=0;i<4;i++){float x=43+i*165;panel(d,x,160,160,52,i==tab);txt(d,f,a,std::string(cats[i])+" ("+std::to_string(count(items,i))+")",x+80,194,17,white,Align::center);}
 for(int i=offset;i<count(items,tab)&&i<offset+11;i++){
   auto* it=nth(items,tab,i);if(!it)continue;float y=233+(i-offset)*68;
   panel(d,44,y,653,65,i==selected);
   d.rounded_rect({58,y+10,47,44},8,color(it->kind==uninstaller::Kind::UsbPackage?0x543046:0x094482));
   txt(d,f,a,it->kind==uninstaller::Kind::UsbPackage?"P":it->kind==uninstaller::Kind::Installed?"G":"H",81,y+40,21,white,Align::center);
   std::string name=it->name.size()>37?it->name.substr(0,34)+"...":it->name;
   txt(d,f,a,name,124,y+29,20);
   txt(d,f,a,it->kind==uninstaller::Kind::UsbPackage?"USB PKG file":it->kind==uninstaller::Kind::Homebrew?"Homebrew app":"Installed game / app",124,y+53,16,muted);
   txt(d,f,a,it->sizeKnown?uninstaller::bytes(it->size):"--",678,y+38,17,muted,Align::right);
 }
 if(count(items,tab)==0)txt(d,f,a,"No accessible titles found",100,300,22,muted);
 const auto* item=nth(items,tab,selected);
 panel(d,740,163,579,273);trash(d,988,237,1.75f);
 if(item){
  txt(d,f,a,item->name.size()>34?item->name.substr(0,31)+"...":item->name,755,480,27);
  txt(d,f,a,item->kind==uninstaller::Kind::UsbPackage?"USB package file":item->kind==uninstaller::Kind::Homebrew?"Homebrew application":"Installed application",755,518,19,muted);
  d.line(758,552,1298,552,1,color(0x36587b));
  txt(d,f,a,"Title ID",761,591,20,muted);txt(d,f,a,item->id.empty()?"Not available":item->id,1290,591,20,white,Align::right);
  txt(d,f,a,"Version",761,630,20,muted);txt(d,f,a,item->version.empty()?"Unknown":item->version,1290,630,20,white,Align::right);
  txt(d,f,a,"Install Size",761,670,20,muted);txt(d,f,a,item->sizeKnown?uninstaller::bytes(item->size):"Not measured",1290,670,20,white,Align::right);
  txt(d,f,a,"Location",761,707,20,muted);
  std::string path=item->root; if(path.size()>30)path=path.substr(0,28)+"...";
  txt(d,f,a,path,1290,707,17,white,Align::right);
  d.bordered_rect({750,747,562,90},14,color(0x880c25),3,color(0xff4366));
  txt(d,f,a,item->kind==uninstaller::Kind::Installed?"Uninstall":"Remove files",1031,806,30,white,Align::center);
  txt(d,f,a,"Save data stays untouched",770,880,20,muted);
  txt(d,f,a,"No system directory is deleted directly",770,926,16,muted);
 }else txt(d,f,a,"Select an application",1025,514,26,muted,Align::center);
 txt(d,f,a,"Console Storage",1400,202,27);
 d.line(1379,227,1860,227,1,color(0x46759e));
 if(disk.available&&disk.total){
  unsigned used=(unsigned)(100*(1.0-(double)disk.free/disk.total));
  txt(d,f,a,std::to_string(used)+"%",1618,405,58,white,Align::center);
  txt(d,f,a,"Used",1618,450,20,muted,Align::center);
  txt(d,f,a,"Total: "+uninstaller::bytes(disk.total),1403,538,22);
  txt(d,f,a,"Used: "+uninstaller::bytes(disk.total-disk.free),1403,585,22);
  txt(d,f,a,"Free: "+uninstaller::bytes(disk.free),1403,631,22);
 }else{
  txt(d,f,a,"Unavailable",1618,414,35,muted,Align::center);
  txt(d,f,a,"Storage API unavailable",1618,480,18,muted,Align::center);
 }
 panel(d,1381,691,478,118);
 txt(d,f,a,"Space to be freed (estimated)",1410,734,20,color(0xff8a9c));
 txt(d,f,a,item&&item->sizeKnown?uninstaller::bytes(item->size):"Unknown",1410,787,37,color(0xff5572));
 trash(d,1548,870,1.7f);
 d.bordered_rect({0,1027,1920,53},0,color(0x020d1a),1,color(0x2b496d));
 txt(d,f,a,"D-pad Select     L1/R1 Categories     X Details     Square Delete     Triangle Refresh     Circle Back",45,1063,18,muted);
 txt(d,f,a,"by YAHBALLON   v0.1.1",1887,1063,18,muted,Align::right);
 if(!msg.empty()){d.rounded_rect({370,944,1170,69},13,color(0x102844));txt(d,f,a,msg.size()>115?msg.substr(0,110)+"...":msg,958,989,19,white,Align::center);}
 if(confirm){
  d.bordered_rect({367,359,1190,322},20,color(0x05162b),3,color(0xff466a));
  txt(d,f,a,"CONFIRM REMOVAL",962,432,39,color(0xff607a),Align::center);
  txt(d,f,a,item?item->name:"Unknown",962,496,25,white,Align::center);
  txt(d,f,a,"X: Delete selected files     Circle: Cancel",962,581,25,white,Align::center);
  txt(d,f,a,"This cannot be undone. Saves are not targeted.",962,636,20,muted,Align::center);
 }
}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;std::string fonts=read("/app0/assets/fonts/inter-semibold.huifont");
 if(fonts.empty())fonts=read("/data/homebrew/PPSA99986/assets/fonts/inter-semibold.huifont");
 if(!font.load(fonts))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 auto items=uninstaller::scan();auto disk=uninstaller::storage();
 int tab=0,selected=0,offset=0;bool confirm=false;
 unsigned prev=0;std::string notice="Ready. Installed titles require a verified system uninstall service.";
 log("Uninstaller v0.1.1 launched");
 for(;;){
  std::array<hui::PadSample,64> s{};unsigned pressed=0,buttons=prev;
  size_t n=pad.read(s);bool connected=false;
  for(size_t i=0;i<n;i++)if(s[i].connected){connected=true;buttons=s[i].buttons;
    if(s[i].left_y<60)buttons|=hui::pad_bits::kUp;
    if(s[i].left_y>195)buttons|=hui::pad_bits::kDown;
    pressed|=buttons&~prev;prev=buttons;
  }
  if(!connected)prev=0;
  if(pressed&hui::pad_bits::kCircle){if(confirm)confirm=false;else{pad.close();renderer.release();display.close();hui::sys::quit();}}
  if(!confirm){
    if(pressed&hui::pad_bits::kL1){tab=(tab+3)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kR1){tab=(tab+1)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kLeft){tab=(tab+3)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kRight){tab=(tab+1)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kUp){if(selected>0)--selected;if(selected<offset)offset=selected;}
    if(pressed&hui::pad_bits::kDown){if(selected+1<count(items,tab))++selected;if(selected>=offset+11)offset=selected-10;}
    if(pressed&hui::pad_bits::kTriangle){items=uninstaller::scan();disk=uninstaller::storage();selected=offset=0;notice="Refreshed.";}
    if(pressed&(hui::pad_bits::kSquare|hui::pad_bits::kCross)){const auto* x=nth(items,tab,selected);if(x)confirm=true;}
  }else if(pressed&hui::pad_bits::kCross){
    auto* x=nth(items,tab,selected);if(x){
      if(x->kind==uninstaller::Kind::Installed){notice="Installed title uninstall requires privileged AppInstUtil integration; not enabled.";log("Installed uninstall blocked: "+x->id);}
      else{
        const int result=uninstaller::removeFiles(*x);
        if(result==0){notice="Selected files removed. Save data not modified.";log("Removed: "+x->path);items=uninstaller::scan();disk=uninstaller::storage();selected=offset=0;}
        else{notice="Remove failed: error "+std::to_string(result);log("Remove failed: "+x->path+" error "+std::to_string(result));}
      }
    }
    confirm=false;
  }
  DrawList ui;render(ui,font,atlas,items,tab,selected,offset,confirm,notice,disk);
  renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
