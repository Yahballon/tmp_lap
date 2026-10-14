// SPDX-License-Identifier: GPL-3.0-or-later
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "ebook_engine.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <vector>
using namespace hui::gfx;
namespace {
const Color cream=Color::rgb(0xffeac8),gold=Color::rgb(0xf4bb60),muted=Color::rgb(0xc49a6d),
 dark=Color::rgb(0x241306),wood=Color::rgb(0x49250f),edge=Color::rgb(0x875022),paper=Color::rgb(0xfff5dd),ink=Color::rgb(0x422b1c);
void txt(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,float size,Color c=cream,Align align=Align::left) {
 d.text(f,a,s,x,y,size,c,align);
}
std::string cut(std::string s,size_t n=31){if(s.size()>n){s.resize(n-3);s+="...";}return s;}
std::string read(const char* p){
 FILE* f=fopen(p,"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 long n=ftell(f);if(n<1||n>10000000){fclose(f);return {};}
 rewind(f);std::string out((size_t)n,0);
 fread(&out[0],1,out.size(),f);fclose(f);return out;
}
void panel(DrawList& d,float x,float y,float w,float h,bool selected=false){
 d.bordered_rect({x,y,w,h},12,selected?Color::rgb(0x5b3416):wood,selected?4:1,selected?gold:edge);
}
void book(DrawList& d,const Font& f,unsigned atlas,const ebook::BookEntry& b,float x,float y,bool selected) {
 d.shadow({x,y,216,282},8,13,Color::rgb(0x080301,0.65f));
 const unsigned colors[]={0x572b35,0x17454c,0x604c27,0x43355e,0x375032,0x614027};
 unsigned hash=0;for(char c:b.title)hash=hash*33+(unsigned char)c;
 Color cover=Color::rgb(colors[hash%6]);
 d.bordered_rect({x,y,208,268},4,cover,selected?5:1,selected?gold:Color::rgb(0xc49557));
 d.rounded_rect({x+7,y+4,14,260},0,Color::rgb(0x170b08,0.6f));
 d.line(x+34,y+23,x+187,y+23,2,gold);
 d.line(x+34,y+237,x+187,y+237,2,gold);
 txt(d,f,atlas,b.format,x+180,y+53,17,gold,Align::right);
 txt(d,f,atlas,cut(b.title,15),x+106,y+129,25,cream,Align::center);
 txt(d,f,atlas,cut(b.author,18),x+106,y+166,16,muted,Align::center);
 if(b.favorite)txt(d,f,atlas,"* FAVOURITE",x+108,y+219,15,gold,Align::center);
 d.rounded_rect({x+35,y+248,139,5},2,Color::rgb(0x130908));
 d.rounded_rect({x+35,y+248,std::min(139.f,139.f*(float)b.progress/10000.f),5},2,gold);
}
std::vector<std::string> dirs(){return {"/data/ebooks","/download0/ebooks","/mnt/usb0/Books","/mnt/usb1/Books","/mnt/usb0","/mnt/usb1","/mnt/usb2"};}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;std::string data=read("/app0/assets/fonts/inter-semibold.huifont");
 if(data.empty())data=read("/data/homebrew/PPSA99741/assets/fonts/inter-semibold.huifont");
 if(!font.load(data))hui::sys::park();
 auto atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 std::vector<ebook::BookEntry> books,imports;
 ebook::scan_books(dirs(),books);ebook::scan_imports(imports);
 ebook::Document document;
 bool reader=false,list=false,confirmDelete=false;
 int selected=0,filter=0,readerPage=0,libraryScroll=0,importSelect=0;
 bool importer=false;std::string message="Select a book to read, or press TRIANGLE to import from USB.";
 const char* names[]={"All","Comics","Fiction","Novels","Favorites"};
 unsigned held=0;int repeatDelay=0;
 for(;;){
  std::array<hui::PadSample,64> samples{};auto n=pad.read(samples);
  unsigned pressed=0,now=held;
  for(size_t i=0;i<n;++i){pressed|=samples[i].buttons&~now;now=samples[i].buttons;}
  held=now;
  if(--repeatDelay<=0){
   if(now&hui::pad_bits::kUp)pressed|=hui::pad_bits::kUp;
   if(now&hui::pad_bits::kDown)pressed|=hui::pad_bits::kDown;
   if(now&hui::pad_bits::kLeft)pressed|=hui::pad_bits::kLeft;
   if(now&hui::pad_bits::kRight)pressed|=hui::pad_bits::kRight;
   repeatDelay=11;
  }
  std::vector<size_t> visible;
  for(size_t i=0;i<books.size();++i){
   bool ok=filter==0||(filter==4?books[i].favorite:books[i].collection==names[filter]);
   if(ok)visible.push_back(i);
  }
  selected=std::clamp(selected,0,std::max(0,(int)visible.size()-1));
  if(pressed&hui::pad_bits::kCircle){
   if(confirmDelete)confirmDelete=false;
   else if(reader){reader=false;ebook::save_state(books);}
   else if(importer)importer=false;
   else {pad.close();renderer.release();display.close();hui::sys::quit();}
  }
  if(reader){
   if((pressed&(hui::pad_bits::kRight|hui::pad_bits::kR1|hui::pad_bits::kCross))
     &&readerPage+1<(int)document.pageCount())++readerPage;
   if((pressed&(hui::pad_bits::kLeft|hui::pad_bits::kL1))&&readerPage>0)--readerPage;
   if(pressed&hui::pad_bits::kR2)readerPage=std::min((int)document.pageCount()-1,readerPage+10);
   if(pressed&hui::pad_bits::kL2)readerPage=std::max(0,readerPage-10);
   if(pressed&hui::pad_bits::kTriangle)readerPage=0;
   if(pressed&hui::pad_bits::kSquare)readerPage=std::max(0,(int)document.pageCount()-1);
   if(selected>=0&&selected<(int)visible.size()){
    auto& b=books[visible[selected]];
    b.progress=(size_t)((readerPage*10000ULL)/std::max<size_t>(1,document.pageCount()));
    if(pressed&(hui::pad_bits::kRight|hui::pad_bits::kLeft|hui::pad_bits::kR1|hui::pad_bits::kL1|hui::pad_bits::kCircle))
       ebook::save_state(books);
   }
  }else if(importer){
   if(pressed&hui::pad_bits::kTriangle){ebook::scan_imports(imports);importSelect=0;}
   if(pressed&hui::pad_bits::kDown)importSelect=std::min((int)imports.size()-1,importSelect+1);
   if(pressed&hui::pad_bits::kUp)importSelect=std::max(0,importSelect-1);
   if((pressed&hui::pad_bits::kCross)&&!imports.empty()){
    ebook::import_to_library(imports[(size_t)importSelect].path,message);
    ebook::scan_books(dirs(),books);ebook::scan_imports(imports);selected=0;
   }
  }else{
   if(pressed&hui::pad_bits::kTriangle){importer=true;ebook::scan_imports(imports);importSelect=0;}
   if(pressed&hui::pad_bits::kOptions){ebook::scan_books(dirs(),books);ebook::scan_imports(imports);selected=0;message="Library scanned: "+std::to_string(books.size())+" books / "+std::to_string(imports.size())+" USB files";}
   if(pressed&hui::pad_bits::kR1){filter=(filter+1)%5;selected=0;}
   if(pressed&hui::pad_bits::kL1){filter=(filter+4)%5;selected=0;}
   if(pressed&hui::pad_bits::kR2)list=!list;
   int step=list?1:5;
   if(pressed&hui::pad_bits::kUp)selected=std::max(0,selected-step);
   if(pressed&hui::pad_bits::kDown)selected=std::min((int)visible.size()-1,selected+step);
   if(pressed&hui::pad_bits::kLeft)selected=std::max(0,selected-1);
   if(pressed&hui::pad_bits::kRight)selected=std::min((int)visible.size()-1,selected+1);
   if((pressed&hui::pad_bits::kSquare)&&!visible.empty()){
    books[visible[selected]].favorite=!books[visible[selected]].favorite;ebook::save_state(books);
   }
   if((pressed&hui::pad_bits::kCross)&&!visible.empty()){
    if(confirmDelete){
     if(ebook::erase_imported(books[visible[selected]]))message="Book removed from /data/ebooks";
     else message="Only imported library files can be deleted";
     confirmDelete=false;ebook::scan_books(dirs(),books);
    }else{
     const auto& b=books[visible[selected]];
     if(ebook::read_document(b.path,document)){
      reader=true;readerPage=std::min<int>((int)document.pageCount()-1,(int)(b.progress*document.pageCount()/10000));
      message="";
     }else message="Cannot open "+b.title+": "+document.error;
    }
   }
   if(pressed&hui::pad_bits::kL3)confirmDelete=!confirmDelete;
  }
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x2b1607),Color::rgb(0x120905));
  // Carved wooden frame, parchment-coloured title bar and ornamental lines.
  d.gradient_rect({0,0,1920,92},0,Color::rgb(0x613719),Color::rgb(0x2d1406));
  d.line(0,91,1920,91,5,gold);
  d.bordered_rect({27,17,58,57},11,Color::rgb(0x281406),2,gold);
  d.line(39,34,55,44,4,gold);d.line(55,44,72,34,4,gold);
  d.line(39,34,39,59,3,gold);d.line(72,34,72,59,3,gold);
  txt(d,font,atlas,"EBOOK READER by YAHBALLON",110,57,30,cream);
  txt(d,font,atlas,"v0.1.1",1873,57,22,gold,Align::right);
  if(reader){
   txt(d,font,atlas,cut(document.title,62),960,141,28,cream,Align::center);
   d.shadow({167,184,1586,758},19,20,Color::rgb(0x050201,.6f));
   d.bordered_rect({161,180,1598,781},12,Color::rgb(0xb89d75),4,edge);
   d.rounded_rect({177,196,1566,750},5,paper);
   d.gradient_rect({952,203,16,728},0,Color::rgb(0xc9bca1),Color::rgb(0xe8dfc9));
   for(int j=0;j<34;++j){
    size_t idx=(size_t)readerPage*34+(size_t)j;
    if(idx>=document.lines.size())break;
    int col=j/17,row=j%17;
    float x=228.f+col*794.f,y=264.f+row*38.f;
    txt(d,font,atlas,cut(document.lines[idx],52),x,y,24,ink);
   }
   txt(d,font,atlas,document.isPdfTextPreview?"PDF text preview - layout not preserved":"EPUB reader",228,231,17,Color::rgb(0x8c653a));
   txt(d,font,atlas,std::to_string(readerPage+1)+" / "+std::to_string(document.pageCount()),960,913,18,ink,Align::center);
   txt(d,font,atlas,"L1/R1  Turn page    L2/R2  Jump 10    TRIANGLE First    SQUARE Last    CIRCLE Bookshelf",960,1037,20,cream,Align::center);
  }else if(importer){
   txt(d,font,atlas,"IMPORT BOOKS FROM USB",95,161,36);
   txt(d,font,atlas,"USB0/Books and nested folders are scanned. Books can also be opened directly from the shelf.",95,211,19,muted);
   if(imports.empty())txt(d,font,atlas,"No readable EPUB / PDF found. Check USB path status on the right.",96,312,23,gold);
   int begin=std::max(0,importSelect-9);
   for(int i=begin;i<(int)imports.size()&&i<begin+9;++i){
    float y=249+(i-begin)*74.f;panel(d,92,y,1168,65,i==importSelect);
    txt(d,font,atlas,cut(imports[i].title,49),115,y+38,22);
    txt(d,font,atlas,imports[i].format,1234,y+38,20,gold,Align::right);
   }
   panel(d,1280,249,562,693);
   txt(d,font,atlas,"USB PATH STATUS",1310,295,24,gold);
   const auto& checks=ebook::last_scan_report();
   for(size_t i=0;i<checks.size()&&i<9;i++){
      const auto& check=checks[i];float y=339+(float)i*65;
      txt(d,font,atlas,cut(check.path,36),1308,y,19,cream);
      const std::string result=check.opened?std::to_string(check.books)+" books, "+std::to_string(check.candidates)+" files":("ERROR: "+check.error);
      txt(d,font,atlas,cut(result,47),1310,y+24,15,check.opened?gold:Color::rgb(0xffa785));
   }
   txt(d,font,atlas,"X Import selected    TRIANGLE Rescan    CIRCLE Back",95,1038,21,cream);
  }else{
   const int count=(int)visible.size();
   d.rounded_rect({53,116,1814,92},9,Color::rgb(0x371c0a));
   txt(d,font,atlas,"MY LIBRARY",92,169,42,cream);
   txt(d,font,atlas,std::to_string(books.size())+" books",480,165,24,muted);
   for(int i=0;i<5;++i){
    float x=840+i*200.f;
    panel(d,x,128,184,59,i==filter);
    txt(d,font,atlas,names[i],x+92,166,19,i==filter?gold:cream,Align::center);
   }
   d.rounded_rect({38,233,1845,767},6,Color::rgb(0x2a1607));
   for(int row=0;row<4;row++){
    float y=270+row*205.f;
    d.gradient_rect({65,y+160,1783,43},0,Color::rgb(0x915829),Color::rgb(0x341808));
    d.line(65,y+160,1848,y+160,3,Color::rgb(0xc48943));
   }
   if(count==0){
      txt(d,font,atlas,"No books in this collection.",960,481,29,muted,Align::center);
      txt(d,font,atlas,"Press TRIANGLE to check USB paths, or OPTIONS to rescan.",960,530,22,gold,Align::center);
   }
   if(count>0){
    int pageSize=list?9:15;
    libraryScroll=selected/pageSize;
    for(int i=libraryScroll*pageSize;i<count&&i<(libraryScroll+1)*pageSize;i++){
     const auto& b=books[visible[i]];
     int index=i-libraryScroll*pageSize;
     if(list){
      float y=260+index*79.f;panel(d,90,y,1715,70,i==selected);
      txt(d,font,atlas,cut(b.title,55),119,y+37,26);
      txt(d,font,atlas,b.format+"  "+b.collection+(b.favorite?"  *":""),1768,y+37,20,gold,Align::right);
     }else{
      float x=151+(index%5)*348.f,y=288+(index/5)*235.f;
      book(d,font,atlas,b,x,y,i==selected);
     }
    }
   }
   txt(d,font,atlas,"COLLECTION: "+std::string(names[filter])+"     "+std::to_string(count)+" books      "+std::to_string(libraryScroll+1)+" / "+std::to_string(std::max(1,(count+(list?8:14))/(list?9:15))),90,995,18,muted);
   txt(d,font,atlas,confirmDelete?"DELETE SELECTED BOOK?   X Confirm   CIRCLE Cancel":
    "X Open   D-PAD Navigate   L1/R1 Collections   R2 List/Grid   SQUARE Favourite   TRIANGLE Import   OPTIONS Scan   L3 Delete",88,1044,18,cream);
  }
  if(!message.empty()){d.rounded_rect({410,1010,1100,35},9,Color::rgb(0x49250f));txt(d,font,atlas,cut(message,118),960,1036,17,gold,Align::center);}
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
