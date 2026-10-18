// SPDX-License-Identifier: GPL-3.0-or-later
// Bible Reader by YAHBALLON v0.2.1 - offline native PS5 homebrew.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#include "bible.hpp"
#include "localization.hpp"
#include "reader_layout.hpp"
#include "tts_reader.hpp"
#include "tts_log.hpp"
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
std::string name(bible::Ref r,int lang){
 const char* localized=bible_i18n::bookName(lang,r.book);
 const std::string label=localized[0]?localized:bible::kBooks[(size_t)r.book];
 return label+" "+std::to_string(r.chapter)+":"+std::to_string(r.verse);
}
bool readTranslation(bible::Library& lib,int lang,std::string& error){
 const std::string code=bible_i18n::code(lang);
 const char* filename=(lang==0||lang==4)?"webpb.tsv":nullptr;
 const std::string file=filename?filename:code+".tsv";
 const std::string appPath="/app0/assets/bible/"+file;
 if(lib.load(appPath,error))return true;
 return lib.load("/data/homebrew/PPSA99793/assets/bible/"+file,error);
}
enum class Page {read,books,chapters,search,results,bookmarks,settings,about};
const char* tabs[]={"Library","Old Testament","New Testament","Bookmarks","Settings","About"};
const char* glyphs[]={"[=]","||","+","[*]","[O]","[i]"};
const bible_i18n::Key tabKeys[]={
 bible_i18n::Key::library,bible_i18n::Key::oldTestament,
 bible_i18n::Key::newTestament,bible_i18n::Key::bookmarks,bible_i18n::Key::settings,
 bible_i18n::Key::about
};
}
int main(){
 mkdir("/data/BibleReader",0755);
 bible_log::write("BOOT","Bible Reader v0.2.0 title=PPSA99793 log=%s",bible_log::path);
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;
 std::string f=read("/app0/assets/fonts/inter-semibold.huifont");
 if(f.empty())f=read("/data/homebrew/PPSA99793/assets/fonts/inter-semibold.huifont");
 if(!font.load(f))hui::sys::park();
 const unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 bible::Settings settings;bible::readSettings(state,settings);
 bible_log::write("SETTINGS","language=%d book=%d chapter=%d verse=%d",settings.language,settings.book,settings.chapter,settings.selectedVerse);
 bible::Library library;std::string problem;
 bible_tts::Speaker voice;
 bool loaded=readTranslation(library,settings.language,problem);
 bible_log::write(loaded?"BIBLE_LOAD_OK":"BIBLE_LOAD_ERR","language=%d details=%s",settings.language,loaded?"offline translation ready":problem.c_str());
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
 std::string query,notice=std::string(bible_i18n::label(settings.language,bible_i18n::Key::offlineStatus))
                 +bible_i18n::translationName(settings.language);
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
  if(voice.active()){voice.stop();return;}
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
   if(pressed&hui::pad_bits::kOptions){voice.stop();page=Page::settings;focusedSidebar=false;listScroll=0;}
   if(pressed&hui::pad_bits::kL1){if(loaded&&page==Page::read){voice.stop();int b=settings.book,c=settings.chapter-1;if(c<1){b=(b+65)%66;c=library.chapterCount(b);}gotoRef({b,c,1});}}
   if(pressed&hui::pad_bits::kR1){if(loaded&&page==Page::read){voice.stop();int b=settings.book,c=settings.chapter+1;if(c>library.chapterCount(b)){b=(b+1)%66;c=1;}gotoRef({b,c,1});}}
   if(pressed&hui::pad_bits::kTriangle){if(page!=Page::search){voice.stop();page=Page::search;query.clear();nav=0;focusedSidebar=false;}}
   if(pressed&hui::pad_bits::kLeft){if(page==Page::read)focusedSidebar=true;else if(page==Page::search)nav=0;}
   if(pressed&hui::pad_bits::kRight){if(page==Page::read)focusedSidebar=false;else if(page==Page::search)nav=1;}
   if(pressed&hui::pad_bits::kUp){
    if(page==Page::read&&focusedSidebar)side=(side+5)%6;
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
    if(page==Page::read&&focusedSidebar)side=(side+1)%6;
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
    else if(page==Page::settings)listScroll=std::min(5,listScroll+1);
    else if(page==Page::search)nav=std::min((int)bible_i18n::searchKeys(settings.language).size()-1,nav+8);
   }
   if(pressed&hui::pad_bits::kSquare){
    if(page==Page::read&&loaded){
     const auto& vv=library.chapter(settings.book,settings.chapter);
     if(selected>=0&&selected<(int)vv.size()){auto& v=vv[(size_t)selected];bible::toggle(settings,{v.book,v.chapter,v.number});save();}
    }else if(page==Page::search)bible_i18n::popUtf8(query);
   }
   if(pressed&hui::pad_bits::kCross){
    if(page==Page::read&&focusedSidebar){
     voice.stop();
     if(side==0)section(0);
     if(side==1)section(0);
     if(side==2)section(39);
     if(side==3){page=Page::bookmarks;listScroll=0;focusedSidebar=false;}
     if(side==4){page=Page::settings;listScroll=0;focusedSidebar=false;}
     if(side==5){page=Page::about;focusedSidebar=false;}
    }else if(page==Page::read&&loaded){
     const auto& vv=library.chapter(settings.book,settings.chapter);
     if(selected>=0&&selected<(int)vv.size()){
      settings.selectedVerse=vv[(size_t)selected].number;save();
      voice.start(library,{settings.book,settings.chapter,settings.selectedVerse},settings.language,settings.voiceStyle==1);
      notice=std::string(bible_i18n::label(settings.language,bible_i18n::Key::reading))+name({settings.book,settings.chapter,settings.selectedVerse},settings.language);
     }
    }else if(page==Page::books){chapterSelection=1;page=Page::chapters;}
    else if(page==Page::chapters&&loaded){gotoRef({bookSelection,chapterSelection,1});}
    else if(page==Page::bookmarks){
     if(listScroll>=0&&listScroll<(int)settings.bookmarks.size())gotoRef(settings.bookmarks[(size_t)listScroll]);
    }else if(page==Page::results){if(listScroll>=0&&listScroll<(int)hits.size())gotoRef(hits[(size_t)listScroll].ref);}
    else if(page==Page::settings){
     voice.stop();
     if(listScroll==0){
      const int candidate=(settings.language+1)%bible_i18n::kLanguageOptions;
      bible::Library replacement;std::string error;
      if(readTranslation(replacement,candidate,error)){
       if(loaded&&!library.chapter(settings.book,settings.chapter).empty())
        settings.selectedVerse=library.chapter(settings.book,settings.chapter)[(size_t)std::clamp(selected,0,(int)library.chapter(settings.book,settings.chapter).size()-1)].number;
       library=std::move(replacement);loaded=true;
       const int oldLanguage=settings.language;
       settings.voiceStyle=bible::voiceStyleAfterLanguageChange(oldLanguage,candidate,settings.voiceStyle);
       settings.language=candidate;
       bible_log::write("TTS_SETTINGS","language_changed=%d->%d voice_style=%d",
                        oldLanguage,candidate,settings.voiceStyle);
       settings.chapter=std::clamp(settings.chapter,1,library.chapterCount(settings.book));
       const auto& vv=library.chapter(settings.book,settings.chapter);
       selected=0;
       for(size_t i=0;i<vv.size();++i)if(vv[i].number==settings.selectedVerse){selected=(int)i;break;}
       verseScroll=std::max(0,selected-2);verseLineScroll=0;
       hits.clear();query.clear();nav=0;
       notice=std::string(bible_i18n::label(settings.language,bible_i18n::Key::offlineStatus))
          +bible_i18n::translationName(settings.language);
      }else notice="Bible translation unavailable: "+error;
     }
     if(listScroll==1)settings.fontSize=settings.fontSize>=38?22:settings.fontSize+2;
     if(listScroll==2)settings.daily=!settings.daily;
     if(listScroll==3)settings.theme=1-settings.theme;
     if(listScroll==4&&settings.language!=4){
      settings.voiceStyle=1-settings.voiceStyle;
      bible_log::write("TTS_SETTINGS","language=%d voice_style=%d",
                       settings.language,settings.voiceStyle);
     }
     if(listScroll==5){page=Page::read;focusedSidebar=true;}
     save();
    }else if(page==Page::search){
     const auto keys=bible_i18n::searchKeys(settings.language);
     const std::string& key=keys[(size_t)std::min(nav,(int)keys.size()-1)];
     if(key=="<")bible_i18n::popUtf8(query);
     else if(query.size()<55)query+=key;
    }
   }
   if(page==Page::search&&(pressed&hui::pad_bits::kR2)){
    hits=loaded?library.search(query):std::vector<bible::SearchHit>{};
    listScroll=0;page=Page::results;
   }
   if(pressed&hui::pad_bits::kL2&&loaded&&page==Page::read){
    std::time_t t=std::time(nullptr);std::tm* date=std::localtime(&t);
    bible::Ref ref=bible::dailyReference(date?date->tm_yday+1:1);
    if(library.verse(ref)){voice.stop();gotoRef(ref);}
   }
  }
  const auto spoken=voice.snapshot();
  if(spoken.active&&loaded&&page==Page::read){
   const auto& current=library.chapter(settings.book,settings.chapter);
   const bool changed=settings.book!=spoken.ref.book||settings.chapter!=spoken.ref.chapter||
       selected<0||selected>=(int)current.size()||
       current[(size_t)selected].number!=spoken.ref.verse;
   if(changed){
    settings.book=spoken.ref.book;settings.chapter=spoken.ref.chapter;
    settings.selectedVerse=spoken.ref.verse;
    const auto& target=library.chapter(settings.book,settings.chapter);
    selected=0;
    for(size_t i=0;i<target.size();i++)if(target[i].number==spoken.ref.verse){selected=(int)i;break;}
    verseScroll=selected;verseLineScroll=0;focusedSidebar=false;save();
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
  const std::string maker="by Y A H B A L L O N";
  text(d,font,atlas,maker,307,148,20,gold);
  text(d,font,atlas,"v0.2.0",307+font.measure(maker,20.f)+34.f,148,19,subdued);
  const float left=39,wide=392;panel(d,left,212,wide,759);
  for(int i=0;i<6;i++){
   float y=i==5?832.f:218.f+i*105.f;
   if(page==Page::read&&focusedSidebar&&side==i)panel(d,40,y,385,91,true);
   text(d,font,atlas,glyphs[i],75,y+59,31,i==side?gold:white);
   text(d,font,atlas,font.fit(bible_i18n::label(settings.language,tabKeys[i]),26,202),155,y+60,26,white);
   text(d,font,atlas,">",400,y+60,27,subdued,Align::right);
  }
  const auto layout=bible_ui::layoutFor(settings.daily);
  const float cx=layout.readerX,cw=layout.readerWidth;
  const float verseTextWidth=layout.verseTextWidth;
  panel(d,cx,210,cw,758);
  if(page==Page::read&&loaded){
   const auto& vv=library.chapter(settings.book,settings.chapter);
   text(d,font,atlas,std::string(bible_i18n::label(settings.language,bible_i18n::Key::bible))+" / "+
         bible_i18n::label(settings.language,settings.book<39?bible_i18n::Key::oldTestament:bible_i18n::Key::newTestament),
        cx+42,271,20,gold);
   text(d,font,atlas,std::string(bible_i18n::bookName(settings.language,settings.book)[0]?bible_i18n::bookName(settings.language,settings.book):bible::kBooks[(size_t)settings.book])+" "+std::to_string(settings.chapter),
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
   // Follow the actual spoken word when a verse wraps onto multiple lines.
   if(spoken.active&&spoken.playing&&spoken.wordEnd>spoken.wordBegin&&
      bible::same(spoken.ref,{settings.book,settings.chapter,vv[(size_t)selected].number})){
    size_t from=0;const auto& lines=wrapped[(size_t)selected];
    for(size_t line=0;line<lines.size();++line){
     size_t at=vv[(size_t)selected].text.find(lines[line],from);
     if(at==std::string::npos)break;
     from=at+lines[line].size();
     if(spoken.wordBegin>=at&&spoken.wordBegin<from){
      if((int)line<verseLineScroll)verseLineScroll=(int)line;
      else if((int)line>=verseLineScroll+maxLines)verseLineScroll=(int)line-maxLines+1;
      break;
     }
    }
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
     const auto& line=lines[(size_t)(first+j)];
     const float size=(float)settings.fontSize;
     text(d,font,atlas,line,cx+110,lineY,size);
     if(i==selected&&spoken.active&&spoken.playing&&spoken.wordEnd>spoken.wordBegin&&
        bible::same(spoken.ref,r)){
      size_t from=0,begin=std::string::npos;
      for(int row=0;row<=first+j;++row){
       size_t at=verse.text.find(lines[(size_t)row],from);
       if(at==std::string::npos)break;
       if(row==first+j)begin=at;
       from=at+lines[(size_t)row].size();
      }
      if(begin!=std::string::npos&&spoken.wordBegin>=begin&&
         spoken.wordEnd<=begin+line.size()){
       const size_t offset=spoken.wordBegin-begin;
       const auto word=line.substr(offset,spoken.wordEnd-spoken.wordBegin);
       const float x=cx+110+font.measure(line.substr(0,offset),size);
       d.rounded_rect({x-2,lineY-size*.76f,font.measure(word,size)+4,size*1.12f},
                      4,Color::rgb(0xffdc27,0.15f));
       text(d,font,atlas,word,x,lineY,size,Color::rgb(0xffdc27));
      }
     }
    }
    if(i==selected&&(int)lines.size()>maxLines){
     text(d,font,atlas,std::to_string(first+1)+"-"+std::to_string(first+visible)+"/"+std::to_string(lines.size()),
          cx+cw-43,y+height-8,16,gold,Align::right);
    }
    y+=height+12.f;
   }
   // Separator and chapter controls live outside the scrollable verse area.
   d.line(cx+43,925,cx+cw-43,925,1,border);
   text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::changeChapter),cx+43,952,17,subdued);
   if(spoken.active){
    d.rounded_rect({cx+cw-335,930,286,29},7,Color::rgb(0x2c391c));
    text(d,font,atlas,spoken.neural?"NEURAL HD - O STOP":bible_i18n::label(settings.language,bible_i18n::Key::ttsStop),cx+cw-322,951,18,Color::rgb(0xffdc27));
   }else if(!spoken.error.empty())
    text(d,font,atlas,font.fit(spoken.error,16,355),cx+cw-401,951,16,gold);
  }else if(!loaded){
   text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::offlineMissing),cx+50,310,34,gold);
   float y=380;wrap(d,font,atlas,problem,cx+50,y,790,25,850);
   text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::reinstall),cx+50,560,20,subdued);
  }else if(page==Page::books){
   text(d,font,atlas,bible_i18n::label(settings.language,firstBook?bible_i18n::Key::newTestament:bible_i18n::Key::oldTestament),cx+45,285,28,gold);
   text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::chooseBook),cx+45,344,40);
   if(bookSelection-firstBook>=11+listScroll)listScroll=bookSelection-firstBook-10;
   for(int i=listScroll;i<listScroll+11;i++){
    int b=firstBook+i;if(b>= (firstBook?66:39))break;
    float y=395+(i-listScroll)*49;
    if(b==bookSelection)panel(d,cx+32,y-35,cw-64,45,true);
    text(d,font,atlas,font.fit(bible_i18n::bookName(settings.language,b)[0]?bible_i18n::bookName(settings.language,b):bible::kBooks[(size_t)b],25,cw-115),cx+62,y,25,b==bookSelection?gold:white);
   }
  }else if(page==Page::chapters){
   text(d,font,atlas,font.fit(bible_i18n::bookName(settings.language,bookSelection)[0]?bible_i18n::bookName(settings.language,bookSelection):bible::kBooks[(size_t)bookSelection],47,cw-110),cx+50,320,47,gold);
   text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::chooseChapter),cx+50,374,27,subdued);
   int cap=library.chapterCount(bookSelection),start=((chapterSelection-1)/48)*48;
   for(int i=0;i<48&&start+i<cap;i++){
    int c=start+i+1;float x=cx+62+(i%6)*138,y=446+(i/6)*60;
    if(c==chapterSelection)panel(d,x-10,y-32,98,48,true);
    text(d,font,atlas,std::to_string(c),x+35,y,26,c==chapterSelection?gold:white,Align::center);
   }
  }else if(page==Page::search){
   text(d,font,atlas,font.fit(bible_i18n::label(settings.language,bible_i18n::Key::searchBible),38,cw-90),cx+45,290,38,gold);
   panel(d,cx+40,321,cw-80,70,true);
   text(d,font,atlas,font.fit(query.empty()?bible_i18n::label(settings.language,bible_i18n::Key::searchHint):query,27,cw-125),cx+60,365,27);
   const auto keys=bible_i18n::searchKeys(settings.language);
   const size_t keyPage=(size_t)(std::max(0,nav)/64)*64;
   for(size_t i=keyPage;i<keys.size()&&i<keyPage+64;i++){
    const size_t relative=i-keyPage;
    float x=cx+47+(relative%8)*100,y=431+(relative/8)*53;
    if((int)i==nav)panel(d,x-11,y-31,81,49,true);
    text(d,font,atlas,keys[i],x+32,y,27,gold,Align::center);
   }
   text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::searchHelp),cx+47,920,20,subdued);
   if(keys.size()>64)text(d,font,atlas,std::to_string(keyPage/64+1)+"/"+std::to_string((keys.size()+63)/64),cx+cw-62,919,18,gold,Align::right);
  }else if(page==Page::results||page==Page::bookmarks){
   const bool search=page==Page::results;
   text(d,font,atlas,search?bible_i18n::label(settings.language,bible_i18n::Key::searchResults):bible_i18n::label(settings.language,bible_i18n::Key::bookmarks),cx+42,295,38,gold);
   const int n=search?(int)hits.size():(int)settings.bookmarks.size();
   text(d,font,atlas,std::to_string(n)+" "+bible_i18n::label(settings.language,search?bible_i18n::Key::matches:bible_i18n::Key::savedVerses),cx+42,341,22,subdued);
   for(int i=listScroll;i<n&&i<listScroll+9;i++){
    float y=402+(i-listScroll)*60;
    if(i==listScroll)panel(d,cx+32,y-37,cw-64,55,true);
    bible::Ref r=search?hits[(size_t)i].ref:settings.bookmarks[(size_t)i];
    text(d,font,atlas,name(r,settings.language),cx+58,y,23,gold);
    const auto* v=library.verse(r);
    if(v)text(d,font,atlas,font.fit(v->text,19,540.f+(cw-927.f)),cx+352,y,19,white);
   }
   if(n==0)text(d,font,atlas,bible_i18n::label(settings.language,search?bible_i18n::Key::noMatches:bible_i18n::Key::noBookmarks),cx+50,473,24,subdued);
  }else if(page==Page::about){
   const float x=cx+50.f;
   text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::about),x,302,44,gold);
   d.line(x,332,cx+cw-50,332,1,border);
   text(d,font,atlas,"BIBLE READER",x,391,40,white);
   text(d,font,atlas,"by YAHBALLON  |  v0.2.0",x,430,23,gold);
   text(d,font,atlas,"Native PS5 Homebrew | 100% Offline",x,490,25,white);
   text(d,font,atlas,"66 books, 1189 chapters, 5 Bible languages.",x,540,23,subdued);
   text(d,font,atlas,"Reading, search, bookmarks and karaoke.",x,583,23,subdued);
   text(d,font,atlas,"Neural HD: Piper VITS / ONNX Runtime.",x,626,23,subdued);
   text(d,font,atlas,"Voices: Kristin, Thorsten, Siwis, DaveFX, Denis.",x,669,23,subdued);
   text(d,font,atlas,"Classic & Retro: eSpeak NG.",x,712,23,subdued);
   text(d,font,atlas,"Bible text: public domain. Voice attribution and",x,755,21,subdued);
   text(d,font,atlas,"licences are included with the installation.",x,788,21,subdued);
   d.line(x,837,cx+cw-50,837,1,border);
   text(d,font,atlas,"O  Back to reading",x,881,23,gold);
  }else if(page==Page::settings){
   text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::settings),cx+48,300,43,gold);
   const bible_i18n::Key labels[]={
    bible_i18n::Key::language,bible_i18n::Key::fontSize,
    bible_i18n::Key::dailyPanel,bible_i18n::Key::colourTheme,
    bible_i18n::Key::voiceStyle,bible_i18n::Key::returnReading
   };
   const std::string vals[]={bible_i18n::languageName(settings.language),
    std::to_string(settings.fontSize),
    bible_i18n::label(settings.language,settings.daily?bible_i18n::Key::on:bible_i18n::Key::off),
    bible_i18n::label(settings.language,settings.theme?bible_i18n::Key::slate:bible_i18n::Key::midnight),
    settings.language==4?"Retro Computer (eSpeak)":bible_i18n::label(settings.language,settings.voiceStyle?bible_i18n::Key::neuralPreview:bible_i18n::Key::classicVoice),""};
   for(int i=0;i<6;i++){
    const float y=379+i*90;
    if(i==listScroll)panel(d,cx+35,y-54,cw-70,80,true);
    text(d,font,atlas,font.fit(bible_i18n::label(settings.language,labels[i]),24,cw*0.57f),cx+70,y,24,white);
    text(d,font,atlas,font.fit(vals[i],23,cw*0.36f),cx+cw-80,y,23,gold,Align::right);
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
  text(d,font,atlas,font.fit(bible_i18n::label(settings.language,bible_i18n::Key::dailyVerse),21,337),1505,354,21,gold);
  d.line(1454,381,1847,381,2,gold);
  if(loaded){
   std::time_t now=std::time(nullptr);std::tm* date=std::localtime(&now);
   bible::Ref ref=bible::dailyReference(date?date->tm_yday+1:1);
   auto v=library.verse(ref);
   if(v){
    float y=455;wrap(d,font,atlas,"\""+v->text+"\"",1452,y,390,26,590,white);
    text(d,font,atlas,name(ref,settings.language),1453,635,19,gold);
   }
  }else text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::offlineMissing),1452,479,22,subdued);
  } // Hide the Daily Verse panel entirely when disabled in settings.
  d.line(0,994,1920,994,1,border);
  text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::controls),
       45,1038,17,subdued);
  if(!notice.empty())text(d,font,atlas,font.fit(notice,19,700),1812,1038,19,gold,Align::right);
  if(exitDialog){
   d.rounded_rect({0,0,1920,1080},0,Color::rgb(0x000309,0.8f));
   panel(d,530,353,860,338,true);
   text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::exitQuestion),960,469,38,gold,Align::center);
   text(d,font,atlas,bible_i18n::label(settings.language,bible_i18n::Key::exitAnswer),960,575,23,white,Align::center);
  }
  renderer.begin();renderer.draw(d);
  renderer.present(0,display.width(),display.height());
  display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
