// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 MicroPython by YAHBALLON v0.1.1
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "port/micropython_embed.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {
const char* const kScripts="/data/PS5MicroPython/scripts";
const size_t kMaxSource=65536, kMaxOutput=24576;
constexpr int kVisibleScripts=8; // Keep lower-left script-folder instructions visible.
alignas(16) char heap[256*1024];
std::string capture;
struct Script {std::string name,path;bool builtin;};
std::vector<Script> library;
std::string readFile(const std::string& path,size_t limit) {
 FILE* f=fopen(path.c_str(),"rb"); if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 long n=ftell(f); if(n<1||static_cast<unsigned long>(n)>limit){fclose(f);return {};}
 rewind(f); std::string s(size_t(n),'\0');
 size_t got=fread(s.data(),1,s.size(),f);fclose(f);
 return got==s.size()?s:std::string();
}
void dirs() {
 mkdir("/data/PS5MicroPython",0755);
 mkdir(kScripts,0755);
 mkdir("/data/PS5MicroPython/logs",0755);
}
bool validName(const std::string& s) {
 return s.size()>3 && s.size()<100 && s[0]!='.' &&
 s.compare(s.size()-3,3,".py")==0 && s.find('/')==std::string::npos &&
 s.find('\\')==std::string::npos;
}
void reload() {
 library.clear();
 for(const char* n:{"hello.py","fibonacci.py","collections.py"})
  library.push_back({n,std::string("/app0/assets/scripts/")+n,true});
 if(DIR* d=opendir(kScripts)){
  while(dirent* e=readdir(d)){
   if(library.size()>=67)break;
   std::string name=e->d_name;
   if(!validName(name))continue;
   std::string path=std::string(kScripts)+"/"+name;
   struct stat st{};
   if(stat(path.c_str(),&st)==0 && S_ISREG(st.st_mode) &&
      st.st_size>0 && st.st_size<=static_cast<off_t>(kMaxSource))
    library.push_back({"USER  "+name,path,false});
  }
  closedir(d);
 }
 if(library.size()>3)
  std::sort(library.begin()+3,library.end(),
   [](const Script& a,const Script& b){return a.name<b.name;});
}
void logOutput(const std::string& name){
 if(FILE* f=fopen("/data/PS5MicroPython/logs/runtime.log","ab")){
  fprintf(f,"\n=== %s ===\n",name.c_str());
  fwrite(capture.data(),1,capture.size(),f);fputc('\n',f);fclose(f);
 }
}
void run(const Script& script,void* stack){
 capture=">>> Running "+script.name+"\n";
 std::string code=readFile(script.path,kMaxSource);
 if(code.empty()&&script.builtin)
  code=readFile(std::string("/data/homebrew/PPSA99791/assets/scripts/")+script.name,kMaxSource);
 if(code.empty()){
  capture+="Could not read Python script (max 64 KiB).\n";logOutput(script.name);return;
 }
 mp_embed_init(heap,sizeof(heap),stack);
 mp_embed_exec_str(code.c_str());
 mp_embed_deinit();
 capture+="\n>>> Finished.\n";logOutput(script.name);
}
std::vector<std::string> getLines(const std::string& s) {
 std::vector<std::string> lines;
 size_t start=0;
 while(start<=s.size()&&lines.size()<1024){
  size_t end=s.find('\n',start);
  if(end==std::string::npos)end=s.size();
  std::string line=s.substr(start,end-start);
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  do{
   size_t n=std::min<size_t>(92,line.size());
   lines.push_back(line.substr(0,n));line.erase(0,n);
  }while(!line.empty()&&lines.size()<1024);
  if(end==s.size())break;
  start=end+1;
 }
 return lines;
}
std::string shortName(const std::string& s){
 return s.size()>32?s.substr(0,29)+"...":s;
}
}
extern "C" void ps5_mp_console_write(const char* bytes,size_t len){
 if(!bytes)return;
 for(size_t i=0;i<len&&capture.size()<kMaxOutput;i++)
  if(bytes[i]!='\r')capture.push_back(bytes[i]);
}
int main(){
 dirs();reload();
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 hui::gfx::Font font;
 std::string fonts=readFile("/app0/assets/fonts/inter-semibold.huifont",16000000);
 if(fonts.empty())fonts=readFile("/data/homebrew/PPSA99791/assets/fonts/inter-semibold.huifont",16000000);
 if(!font.load(fonts))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 int selected=0,scroll=0,stack_anchor=0;unsigned previous=0;
 unsigned long long repeatAt=0;
 capture="MicroPython is ready.\nSelect a script and press X to run.\n";
 using namespace hui::gfx;
 const Color fg=Color::rgb(0xeef5ff),cyan=Color::rgb(0x5fe5fb);
 const Color mute=Color::rgb(0x8fa8ca),edge=Color::rgb(0x233d5d);
 for(;;){
  std::array<hui::PadSample,64> samples{};
  auto n=pad.read(samples);unsigned keys=0,pressed=0;bool connected=false;
  for(size_t i=0;i<n;i++)if(samples[i].connected){
   connected=true;keys=samples[i].buttons;
   if(samples[i].left_y<64)keys|=hui::pad_bits::kUp;
   if(samples[i].left_y>192)keys|=hui::pad_bits::kDown;
   pressed|=keys&~previous;previous=keys;
  }
  if(!connected)previous=0;
  auto now=static_cast<unsigned long long>(hui::sys::monotonic_us());
  unsigned dirs=keys&(hui::pad_bits::kUp|hui::pad_bits::kDown);
  unsigned nav=pressed&dirs;
  if(nav)repeatAt=now+320000;
  else if(dirs&&now>=repeatAt){nav=dirs;repeatAt=now+120000;}
  else if(!dirs)repeatAt=0;
  if((nav&hui::pad_bits::kUp)&&selected>0)--selected;
  if((nav&hui::pad_bits::kDown)&&selected+1<int(library.size()))++selected;
  if(pressed&hui::pad_bits::kTriangle){
   reload();selected=std::min(selected,std::max(0,int(library.size())-1));
   capture+="\nRefreshed script list.\n";
  }
  if(pressed&hui::pad_bits::kSquare){capture.clear();scroll=0;}
  if(pressed&hui::pad_bits::kCross){
   if(!library.empty())run(library[size_t(selected)],&stack_anchor);
   scroll=0;
  }
  if(pressed&hui::pad_bits::kL1)scroll=std::max(0,scroll-8);
  if(pressed&hui::pad_bits::kR1)scroll+=8;
  if(pressed&hui::pad_bits::kCircle){
   pad.close();renderer.release();display.close();hui::sys::quit();
  }
  DrawList ui;
  ui.gradient_rect({0,0,1920,1080},0,Color::rgb(0x0b1427),Color::rgb(0x040710));
  ui.rounded_rect({48,45,1824,103},18,Color::rgb(0x101d34));
  ui.rounded_rect({70,68,9,57},4,cyan);
  ui.text(font,atlas,"PS5 MicroPython",103,116,47,fg);
  ui.text(font,atlas,"by YAHBALLON   v0.1.1",1838,109,23,mute,Align::right);
  ui.bordered_rect({48,171,610,811},19,Color::rgb(0x0c172b),2,edge);
  ui.bordered_rect({679,171,1193,811},19,Color::rgb(0x07101e),2,edge);
  ui.text(font,atlas,"PYTHON SCRIPTS",84,228,26,cyan);
  ui.text(font,atlas,"CONSOLE OUTPUT",721,228,26,cyan);
  ui.text(font,atlas,"MicroPython Embed   |   256 KB heap",1833,229,18,mute,Align::right);
  ui.line(84,251,627,251,2,edge);ui.line(721,251,1830,251,2,edge);
  int first=std::max(0,selected-(kVisibleScripts-1));
  for(int i=first;i<int(library.size())&&i<first+kVisibleScripts;i++){
   int y=274+(i-first)*53;bool active=(i==selected);
   ui.bordered_rect({80.f,float(y),543,46},9,active?Color::rgb(0x193a61):Color::rgb(0x102039),
                    active?2.f:1.f,active?cyan:edge);
   ui.text(font,atlas,active?">":" ",99,y+30,22,cyan);
   ui.text(font,atlas,shortName(library[size_t(i)].name),130,y+30,22,fg);
   ui.text(font,atlas,library[size_t(i)].builtin?"DEMO":"LOCAL",
           600,y+29,15,mute,Align::right);
  }
  // Persistent guidance remains visible even after executing a Python script.
  // Eight script rows end at y=691; this card starts at y=726 with no overlap.
  ui.bordered_rect({80,726,543,233},14,Color::rgb(0x0d2540),2,edge);
  ui.text(font,atlas,"ADD YOUR OWN SCRIPTS",105,766,23,cyan);
  ui.line(105,784,597,784,1,edge);
  ui.text(font,atlas,"Place your .py files here:",105,821,20,fg);
  ui.text(font,atlas,"/data/PS5MicroPython/scripts/",105,857,20,cyan);
  ui.text(font,atlas,"Transfer via FTP or a file manager.",105,895,18,mute);
  ui.text(font,atlas,"Press TRIANGLE to refresh the list.",105,930,18,mute);
  auto lines=getLines(capture);
  const int visible=24, last=std::max(0,int(lines.size())-visible);
  scroll=std::min(scroll,last);
  int start=std::max(0,last-scroll);
  for(int i=0;i<visible&&start+i<int(lines.size());i++)
   ui.text(font,atlas,lines[size_t(start+i)],722,290.f+i*27.2f,20,fg);
  ui.rounded_rect({48,996,1824,61},13,Color::rgb(0x101d34));
  ui.text(font,atlas,
   "D-PAD  Select     X  Run     TRIANGLE  Reload     SQUARE  Clear     L1/R1  Scroll     O  Exit",
   960,1035,19,mute,Align::center);
  renderer.begin();renderer.draw(ui);
  renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
