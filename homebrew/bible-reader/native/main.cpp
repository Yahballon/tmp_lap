// SPDX-License-Identifier: GPL-3.0-or-later
// Bible Reader by YAHBALLON v0.1.2 - offline native PS5 homebrew.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#include "bible.hpp"
#include "reader_layout.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <sys/stat.h>
using namespace hui::gfx;
namespace {
const Color white=Color::rgb(0xf6f5fa), subdued=Color::rgb(0xb9c5d9);
const Color gold=Color::rgb(0xeec777), blue=Color::rgb(0x1597ff);
const Color border=Color::rgb(0x315277), navy=Color::rgb(0x05142c);
const char* state="/data/BibleReader/settings.txt";
std::string read(const char* path){
 FILE* f=std::fopen(path,"rb");if(!f)return {};
 if(std::fseek(f,0,SEEK_END)){std::fclose(f);return {};}
 long n=std::ftell(f);if(n<1||n>10000000){std::fclose(f);return {};}
 std::rewind(f);std::string s((size_t)n,0);
 if(std::fread(s.data(),1,s.size(),f)!=(size_t)n)s.clear();
 std::fclose(f);return s;
}
void text(DrawList& d,const Font& f,unsigned atlas,const std::string& s,float x,float y,float size,Color color=white,Align a=Align::left){
 d.text(f,atlas,s,x,y,size,color,a);
}
void panel(DrawList& d,float x,float y,float w,float h,bool focus=false){
 if(focus)d.glow({x-2,y-2,w+4,h+4},12,6,blue);
 d.bordered_rect({x,y,w,h},12,focus?Color::rgb(0x082956):Color::rgb(0x05162e),
                 focus?2.5f:1.0f,focus?blue:border);
}
void wrap(DrawList& d,const Font& font,unsigned atlas,const std::string& str,float x,float& y,
          float width,float fontSize,float bottom,Color color=white){
 for(const auto& line:font.wrap(str,fontSize,width)){
  if(y>bottom)return;
  text(d,font,atlas,line,x,y,fontSize,color);
  y+=fontSize*1.40f;
 }
}
std::string name(bible::Ref r){
 return std::string(bible::kBooks[(size_t)r.book])+" "+std::to_string(r.chapter)+":"+std::to_string(r.verse);
}
enum class Page {read,books,chapters,search,results,bookmarks,settings};
const char* tabs[]={"Library","Old Testament","New Testament","Bookmarks","Settings"};
const char* glyphs[]={"[=]","||","+","[*]","[O]"};
}
int main(){
 mkdir("/data/BibleReader",0755);
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;
 std::string f=read("/app0/assets/fonts/inter-semibold.huifont");
 if(f.empty())f=read("/data/homebrew/PPSA99793/assets/fonts/inter-semibold.huifont");
 if(!font.load(f))hui::sys::park();
 const unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 bible::Library library;std::string problem;
 bool loaded=library.load("/app0/assets/bible/webpb.tsv",problem);
 if(!loaded)loaded=library.load("/data/homebrew/PPSA99793/assets/bible/webpb.tsv",problem);
 bible::Settings settings;bible::readSettings(state,settings);
 if(loaded){
  settings.book=std::clamp(settings.book,0,65);
  settings.chapter=std::clamp(settings.chapter,1,library.chapterCount(settings.book));
 }
 Page page=Page::read;int nav=0,side=0,selected=0,verseScroll=0,verseLineScroll=0,listScroll=0;
 int firstBook=0,bookSelection=0,chapterSelection=1;
 if(loaded){
  const auto& current=library.chapter(settings.book,settings.chapter);
  for(size_t i=0;i<current.size();++i){
   if(current[i].number==settings.selectedVerse){selected=(int)i;break;}
  }
 }
 std::string query,notice="100% offline - World English Bible (WEBPB)";
 std::vector<bible::SearchHit> hits;
 bool exitDialog=false,focusedSidebar=true;
 unsigned previous=0;
 auto save=[&](){bible::writeSettings(state,settings);};
 auto gotoRef=[&](bible::Ref r){
  settings.book=r.book;settings.chapter=r.chapter;settings.selectedVerse=r.verse;
  selected=0;verseScroll=0;verseLineScroll=0;page=Page::read;focusedSidebar=false;
  const auto& vv=library.chapter(r.book,r.chapter);
  for(size_t i=0;i<vv.size();i++)if(vv[i].number==r.verse){selected=(int)i;break;}
  verseScroll=std::max(0,selected-2);save();
 };
 auto section=[&](int b){
  firstBook=b;bookSelection=b;listScroll=0;page=Page::books;focusedSidebar=false;
 };
 auto back=[&](){
  if(exitDialog){exitDialog=false;return;}
  if(page==Page::read){if(focusedSidebar)exitDialog=true;else focusedSidebar=true;}
  else if(page==Page::chapters)page=Page::books;
  else {page=Page::read;focusedSidebar=true;}
 };
 for(;;){
  std::array<hui::PadSample,64> samples{};unsigned pressed=0;
  size_t count=pad.read(samples);
  for(size_t i=0;i<count;i++){pressed|=samples[i].buttons&~previous;previous=samples[i].buttons;}
  if(count==0)previous=0;
  if(exitDialog){
   if(pressed&hui::pad_bits::kCircle)exitDialog=false;
   if(pressed&hui::pad_bits::kCross){save();pad.close();renderer.release();display.close();hui::sys::quit();}
  }else{
   if(pressed&hui::pad_bits::kCircle)back();
   if(pressed&hui::pad_bits::kOptions){page=Page::settings;focusedSidebar=false;listScroll=0;}
   if(pressed&hui::pad_bits::kL1){if(loaded&&page==Page::read){int b=settings.book,c=settings.chapter-1;if(c<1){b=(b+65)%66;c=library.chapterCount(b);}gotoRef({b,c,1});}}
   if(pressed&hui::pad_bits::kR1){if(loaded&&page==Page::read){int b=settings.book,c=settings.chapter+1;if(c>library.chapterCount(b)){b=(b+1)%66;c=1;}gotoRef({b,c,1});}}
   if(pressed&hui::pad_bits::kTriangle){if(page!=Page::search){page=Page::search;query.clear();nav=0;focusedSidebar=false;}}
   if(pressed&hui::pad_bits::kLeft){if(page==Page::read)focusedSidebar=true;else if(page==Page::search)nav=0;}
   if(pressed&hui::pad_bits::kRight){if(page==Page::read)focusedSidebar=false;else if(page==Page::search)nav=1;}
   if(pressed&hui::pad_bits::kUp){
    if(page==Page::read&&focusedSidebar)side=(side+4)%5;
    else if(page==Page::read&&loaded){
     if(verseLineScroll>0)--verseLineScroll;
     else {selected=std::max(0,selected-1);verseScroll=std::min(verseScroll,selected);verseLineScroll=0;}
    }
    else if(page==Page::books){bookSelection=std::max(firstBook,bookSelection-1);listScroll=std::min(listScroll,bookSelection-firstBook);}
    else if(page==Page::chapters)chapterSelection=std::max(1,chapterSelection-6);
    else if(page==Page::results||page==Page::bookmarks||page==Page::settings)listScroll=std::max(0,listScroll-1);
    else if(page==Page::search)nav=std::max(0,nav-8);
   }
   if(pressed&hui::pad_bits::kDown){
    if(page==Page::read&&focusedSidebar)side=(side+1)%5;
    else if(page==Page::read&&loaded){
     const auto& verses=library.chapter(settings.book,settings.chapter);
     const int maxLines=std::max(1,(int)((899.f-454.f-32.f)/(settings.fontSize*1.40f)));
     const int total=(int)font.wrap(verses[(size_t)selected].text,(float)settings.fontSize,bible_ui::layoutFor(settings.daily).verseTextWidth).size();
     if(total>maxLines&&verseLineScroll<total-maxLines)++verseLineScroll;
     else {selected=std::min((int)verses.size()-1,selected+1);verseLineScroll=0;}
    }
    else if(page==Page::books){bookSelection=std::min(firstBook==0?38:65,bookSelection+1);}
    else if(page==Page::chapters&&loaded)chapterSelection=std::min(library.chapterCount(bookSelection),chapterSelection+6);
    else if(page==Page::results)listScroll=std::min((int)hits.size()-1,listScroll+1);
    else if(page==Page::bookmarks)listScroll=std::min((int)settings.bookmarks.size()-1,listScroll+1);
    else if(page==Page::settings)listScroll=std::min(3,listScroll+1);
    else if(page==Page::search)nav=std::min(41,nav+8);
   }
   if(pressed&hui::pad_bits::kSquare){
    if(page==Page::read&&loaded){
     const auto& vv=library.chapter(settings.book,settings.chapter);
     if(selected>=0&&selected<(int)vv.size()){auto& v=vv[(size_t)selected];bible::toggle(settings,{v.book,v.chapter,v.number});save();}
    }else if(page==Page::search){if(!query.empty())query.pop_back();}
   }
   if(pressed&hui::pad_bits::kCross){
    if(page==Page::read&&focusedSidebar){
     if(side==0)section(0);
     if(side==1)section(0);
     if(side==2)section(39);
     if(side==3){page=Page::bookmarks;listScroll=0;focusedSidebar=false;}
     if(side==4){page=Page::settings;listScroll=0;focusedSidebar=false;}
    }else if(page==Page::read&&loaded){
     const auto& vv=library.chapter(settings.book,settings.chapter);
     if(selected>=0&&selected<(int)vv.size()){
      settings.selectedVerse=vv[(size_t)selected].number;save();
      notice="Reading "+name({settings.book,settings.chapter,settings.selectedVerse});
     }
    }else if(page==Page::books){chapterSelection=1;page=Page::chapters;}
    else if(page==Page::chapters&&loaded){gotoRef({bookSelection,chapterSelection,1});}
    else if(page==Page::bookmarks){
     if(listScroll>=0&&listScroll<(int)settings.bookmarks.size())gotoRef(settings.bookmarks[(size_t)listScroll]);
    }else if(page==Page::results){if(listScroll>=0&&listScroll<(int)hits.size())gotoRef(hits[(size_t)listScroll].ref);}
    else if(page==Page::settings){
     if(listScroll==0)settings.fontSize=settings.fontSize>=38?22:settings.fontSize+2;
     if(listScroll==1)settings.daily=!settings.daily;
     if(listScroll==2)settings.theme=1-settings.theme;
     if(listScroll==3){page=Page::read;focusedSidebar=true;}
     save();
    }else if(page==Page::search){
     const std::string keys="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -'<";
     char c=keys[(size_t)std::min(nav,(int)keys.size()-1)];
     if(c=='<'){if(!query.empty())query.pop_back();}
     else if(query.size()<55)query+=(char)std::tolower((unsigned char)c);
    }
   }
   if(page==Page::search&&(pressed&hui::pad_bits::kR2)){
    hits=loaded?library.search(query):std::vector<bible::SearchHit>{};
    listScroll=0;page=Page::results;
   }
   if(pressed&hui::pad_bits::kL2&&loaded&&page==Page::read){
    std::time_t t=std::time(nullptr);std::tm* date=std::localtime(&t);
    bible::Ref ref=bible::dailyReference(date?date->tm_yday+1:1);
    if(library.verse(ref))gotoRef(ref);
   }
  }
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,Color::rgb(settings.theme?0x142342:0x031129),
                  Color::rgb(0x01050e));
  d.circle(1620,125,142,Color::rgb(0x12264b,0.42f));
  d.circle(1620,125,94,Color::rgb(0xd6a34c,0.16f));
  d.line(1620,47,1620,196,9,gold);
  d.line(1573,111,1667,111,9,gold);
  d.rounded_rect({0,0,1920,207},0,Color::rgb(0x081731,0.42f));
  text(d,font,atlas,"+  B I B L E   R E A D E R",80,100,56,gold);
  text(d,font,atlas,"by Y A H B A L L O N",307,148,20,gold);
  text(d,font,atlas,"v0.1.2",1825,89,19,subdued,Align::right);
  const float left=39,wide=392;panel(d,left,212,wide,759);
  for(int i=0;i<5;i++){
   float y=218+i*105;
   if(page==Page::read&&focusedSidebar&&side==i)panel(d,40,y,385,91,true);
   text(d,font,atlas,glyphs[i],75,y+59,31,i==side?gold:white);
   text(d,font,atlas,tabs[i],155,y+60,28,white);
   text(d,font,atlas,">",400,y+60,27,subdued,Align::right);
  }
  const auto layout=bible_ui::layoutFor(settings.daily);
  const float cx=layout.readerX,cw=layout.readerWidth;
  const float verseTextWidth=layout.verseTextWidth;
  panel(d,cx,210,cw,758);
  if(page==Page::read&&loaded){
   const auto& vv=library.chapter(settings.book,settings.chapter);
   text(d,font,atlas,"B I B L E  /  "+std::string(settings.book<39?"OLD TESTAMENT":"NEW TESTAMENT"),
        cx+42,271,20,gold);
   text(d,font,atlas,std::string(bible::kBooks[(size_t)settings.book])+" "+std::to_string(settings.chapter),
        cx+42,357,59,white);
   d.line(cx+42,411,cx+cw-42,411,1,border);
   // Fit the highlighted verse fully into the reading viewport instead
   // of drawing its border over the lower chapter hint or outside the panel.
   constexpr float kTextTop=454.f, kTextBottom=906.f;
   const float lineHeight=settings.fontSize*1.40f;
   const int maxLines=std::max(1,(int)((kTextBottom-kTextTop-32.f)/lineHeight));
   const int end=(int)vv.size();
   selected=std::clamp(selected,0,end-1);
   verseScroll=std::clamp(verseScroll,0,selected);
   std::vector<std::vector<std::string>> wrapped((size_t)end);
   std::vector<float> heights((size_t)end);
   for(int i=0;i<end;++i){
    wrapped[(size_t)i]=font.wrap(vv[(size_t)i].text,(float)settings.fontSize,verseTextWidth);
    const int count=(int)wrapped[(size_t)i].size();
    const int visible=i==selected?std::min(count,maxLines):count;
    heights[(size_t)i]=visible*lineHeight+18.f;
   }
   // Make the selected verse visible on every D-pad step, even when
   // preceding verses have different paragraph heights.
   float needed=0;
   for(int i=verseScroll;i<=selected;++i)needed+=heights[(size_t)i]+12.f;
   while(needed>kTextBottom-kTextTop && verseScroll<selected){
    needed-=heights[(size_t)verseScroll]+12.f;
    ++verseScroll;
   }
   float y=kTextTop;
   for(int i=verseScroll;i<end;++i){
    const auto& verse=vv[(size_t)i];
    const auto& lines=wrapped[(size_t)i];
    const float height=heights[(size_t)i];
    if(y+height>kTextBottom)break;
    if(i==selected)panel(d,cx+35,y-19,cw-70,height+5.f,!focusedSidebar);
    auto r=bible::Ref{verse.book,verse.chapter,verse.number};
    text(d,font,atlas,std::to_string(verse.number),cx+58,y+12,22,gold);
    if(bible::bookmarked(settings,r))text(d,font,atlas,"*",cx+8,y+12,25,gold);
    const int first=i==selected?std::clamp(verseLineScroll,0,std::max(0,(int)lines.size()-maxLines)):0;
    const int visible=std::min((int)lines.size()-first,i==selected?maxLines:(int)lines.size());
    for(int j=0;j<visible;++j){
     const float lineY=y+12.f+j*lineHeight;
     if(lineY>kTextBottom-4.f)break;
     text(d,font,atlas,lines[(size_t)(first+j)],cx+110,lineY,(float)settings.fontSize);
    }
    if(i==selected&&(int)lines.size()>maxLines){
     text(d,font,atlas,std::to_string(first+1)+"-"+std::to_string(first+visible)+"/"+std::to_string(lines.size()),
          cx+cw-43,y+height-8,16,gold,Align::right);
    }
    y+=height+12.f;
   }
   // Separator and chapter controls live outside the scrollable verse area.
   d.line(cx+43,925,cx+cw-43,925,1,border);
   text(d,font,atlas,"L1 / R1  Change chapter",cx+43,952,17,subdued);
  }else if(!loaded){
   text(d,font,atlas,"Bible data missing",cx+50,310,42,gold);
   float y=380;wrap(d,font,atlas,problem,cx+50,y,790,25,850);
   text(d,font,atlas,"Reinstall a complete build containing assets/bible/webpb.tsv",cx+50,560,20,subdued);
  }else if(page==Page::books){
   text(d,font,atlas,firstBook?"NEW TESTAMENT":"OLD TESTAMENT",cx+45,285,28,gold);
   text(d,font,atlas,"Choose a book",cx+45,344,46);
   if(bookSelection-firstBook>=11+listScroll)listScroll=bookSelection-firstBook-10;
   for(int i=listScroll;i<listScroll+11;i++){
    int b=firstBook+i;if(b>= (firstBook?66:39))break;
    float y=395+(i-listScroll)*49;
    if(b==bookSelection)panel(d,cx+32,y-35,cw-64,45,true);
    text(d,font,atlas,bible::kBooks[(size_t)b],cx+62,y,25,b==bookSelection?gold:white);
   }
  }else if(page==Page::chapters){
   text(d,font,atlas,bible::kBooks[(size_t)bookSelection],cx+50,320,49,gold);
   text(d,font,atlas,"Choose a chapter",cx+50,374,27,subdued);
   int cap=library.chapterCount(bookSelection),start=((chapterSelection-1)/48)*48;
   for(int i=0;i<48&&start+i<cap;i++){
    int c=start+i+1;float x=cx+62+(i%6)*138,y=446+(i/6)*60;
    if(c==chapterSelection)panel(d,x-10,y-32,98,48,true);
    text(d,font,atlas,std::to_string(c),x+35,y,26,c==chapterSelection?gold:white,Align::center);
   }
  }else if(page==Page::search){
   text(d,font,atlas,"SEARCH THE BIBLE",cx+45,290,42,gold);
   panel(d,cx+40,321,cw-80,70,true);
   text(d,font,atlas,query.empty()?"Enter a word or phrase":query,cx+60,365,27);
   const std::string keys="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -'<";
   for(size_t i=0;i<keys.size();i++){
    float x=cx+47+(i%8)*100,y=447+(i/8)*73;
    if((int)i==nav)panel(d,x-11,y-36,81,59,true);
    text(d,font,atlas,std::string(1,keys[i]),x+32,y,30,gold,Align::center);
   }
   text(d,font,atlas,"R2  Search     SQUARE  Delete     O  Back",cx+47,920,21,subdued);
  }else if(page==Page::results||page==Page::bookmarks){
   const bool search=page==Page::results;
   text(d,font,atlas,search?"SEARCH RESULTS":"BOOKMARKS",cx+42,295,43,gold);
   const int n=search?(int)hits.size():(int)settings.bookmarks.size();
   text(d,font,atlas,std::to_string(n)+(search?" matches (up to 150)":" saved verses"),cx+42,341,22,subdued);
   for(int i=listScroll;i<n&&i<listScroll+9;i++){
    float y=402+(i-listScroll)*60;
    if(i==listScroll)panel(d,cx+32,y-37,cw-64,55,true);
    bible::Ref r=search?hits[(size_t)i].ref:settings.bookmarks[(size_t)i];
    text(d,font,atlas,name(r),cx+58,y,23,gold);
    const auto* v=library.verse(r);
    if(v)text(d,font,atlas,font.fit(v->text,19,540.f+(cw-927.f)),cx+352,y,19,white);
   }
   if(n==0)text(d,font,atlas,search?"No matching verses.":"No bookmarks yet.",cx+50,473,24,subdued);
  }else if(page==Page::settings){
   text(d,font,atlas,"SETTINGS",cx+48,300,44,gold);
   std::string labels[]={"Font size","Daily verse panel","Colour theme","Return to reading"};
   std::string vals[]={std::to_string(settings.fontSize),settings.daily?"On":"Off",settings.theme?"Slate":"Midnight",""};
   for(int i=0;i<4;i++){
    float y=394+i*105;
    if(i==listScroll)panel(d,cx+35,y-54,cw-70,81,true);
    text(d,font,atlas,labels[i],cx+70,y,28,white);
    text(d,font,atlas,vals[i],cx+cw-80,y,25,gold,Align::right);
   }
  }
  if(layout.showDaily){
  panel(d,1417,295,468,371);
  // Draw the sun as geometry; the baked UI font has no U+263C sun glyph.
  d.circle(1464,336,11,gold);
  for(int k=0;k<8;++k){
   const float a=(float)k*3.14159265f/4.f;
   d.line(1464+std::cos(a)*17.f,336+std::sin(a)*17.f,
          1464+std::cos(a)*25.f,336+std::sin(a)*25.f,2,gold);
  }
  text(d,font,atlas,"D A I L Y   V E R S E",1505,354,24,gold);
  d.line(1454,381,1847,381,2,gold);
  if(loaded){
   std::time_t now=std::time(nullptr);std::tm* date=std::localtime(&now);
   bible::Ref ref=bible::dailyReference(date?date->tm_yday+1:1);
   auto v=library.verse(ref);
   if(v){
    float y=455;wrap(d,font,atlas,"\""+v->text+"\"",1452,y,390,26,590,white);
    text(d,font,atlas,name(ref),1453,635,19,gold);
   }
  }else text(d,font,atlas,"Offline Bible unavailable",1452,479,25,subdued);
  } // Hide the Daily Verse panel entirely when disabled in settings.
  d.line(0,994,1920,994,1,border);
  text(d,font,atlas,"X Select   O Back   SQUARE Bookmark   TRIANGLE Search   L1/R1 Chapters   OPTIONS Settings   L2 Daily verse",
       45,1038,21,subdued);
  if(!notice.empty())text(d,font,atlas,font.fit(notice,19,700),1812,1038,19,gold,Align::right);
  if(exitDialog){
   d.rounded_rect({0,0,1920,1080},0,Color::rgb(0x000309,0.8f));
   panel(d,530,353,860,338,true);
   text(d,font,atlas,"EXIT BIBLE READER?",960,469,43,gold,Align::center);
   text(d,font,atlas,"X  Yes, close app     O  No, continue reading",960,575,26,white,Align::center);
  }
  renderer.begin();renderer.draw(d);
  renderer.present(0,display.width(),display.height());
  display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
