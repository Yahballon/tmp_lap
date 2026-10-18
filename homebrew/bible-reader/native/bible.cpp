// SPDX-License-Identifier: GPL-3.0-or-later
#include "bible.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
namespace bible {
const std::array<const char*,66> kBooks={{
 "Genesis","Exodus","Leviticus","Numbers","Deuteronomy","Joshua","Judges","Ruth",
 "1 Samuel","2 Samuel","1 Kings","2 Kings","1 Chronicles","2 Chronicles","Ezra","Nehemiah",
 "Esther","Job","Psalms","Proverbs","Ecclesiastes","Song of Solomon","Isaiah","Jeremiah",
 "Lamentations","Ezekiel","Daniel","Hosea","Joel","Amos","Obadiah","Jonah","Micah",
 "Nahum","Habakkuk","Zephaniah","Haggai","Zechariah","Malachi","Matthew","Mark",
 "Luke","John","Acts","Romans","1 Corinthians","2 Corinthians","Galatians","Ephesians",
 "Philippians","Colossians","1 Thessalonians","2 Thessalonians","1 Timothy","2 Timothy",
 "Titus","Philemon","Hebrews","James","1 Peter","2 Peter","1 John","2 John","3 John",
 "Jude","Revelation"
}};
static std::string lower(std::string s){
 for(size_t i=0;i<s.size();++i){
  unsigned char c=(unsigned char)s[i];
  if(c<128){s[i]=(char)std::tolower(c);continue;}
  // UTF-8 Latin-1 accented uppercase (À..Ö, Ø..Þ), maintaining byte width.
  // Russian uppercase Cyrillic is UTF-8 D0 xx: fold А-Я and Ё.
  if(c==0xd0&&i+1<s.size()){
   unsigned char following=(unsigned char)s[i+1];
   if(following==0x81){s[i]=(char)0xd1;s[i+1]=(char)0x91;++i;continue;}
   if(following>=0x90&&following<=0x9f){s[i+1]=(char)(following+0x20);++i;continue;}
   if(following>=0xa0&&following<=0xaf){s[i]=(char)0xd1;s[i+1]=(char)(following-0x20);++i;continue;}
  }
  if(c==0xc3&&i+1<s.size()){
   unsigned char b=(unsigned char)s[i+1];
   if((b>=0x80&&b<=0x96)||(b>=0x98&&b<=0x9e)){s[i+1]=(char)(b+0x20);++i;}
  }
 }
 return s;
}
static const std::vector<Verse> empty;
int Library::chapterCount(int book) const {
 if(book<0||book>=66)return 0;
 return (int)data_[(size_t)book].size()-1;
}
const std::vector<Verse>& Library::chapter(int book,int ch) const {
 if(book<0||book>=66||ch<1||ch>chapterCount(book))return empty;
 return data_[(size_t)book][(size_t)ch];
}
const Verse* Library::verse(Ref r) const {
 for(const Verse& v:chapter(r.book,r.chapter))if(v.number==r.verse)return &v;
 return nullptr;
}
bool Library::load(const std::string& path,std::string& error){
 count_=0;
 for(auto& x:data_)x.clear();
 FILE* f=std::fopen(path.c_str(),"rb");
 if(!f){error="Offline Bible file unavailable: "+path;return false;}
 char row[32768];size_t rows=0;bool bad=false;
 while(std::fgets(row,sizeof(row),f)){
  const size_t n=std::strlen(row);
  if(n==0)continue;
  if(row[n-1]!='\n'&&!std::feof(f)){bad=true;break;}
  if(row[0]=='#')continue;
  char* p1=std::strchr(row,'\t');if(!p1){bad=true;break;}*p1++=0;
  char* p2=std::strchr(p1,'\t');if(!p2){bad=true;break;}*p2++=0;
  char* p3=std::strchr(p2,'\t');if(!p3){bad=true;break;}*p3++=0;
  char* end=nullptr;long b=std::strtol(row,&end,10);if(!end||*end){bad=true;break;}
  long c=std::strtol(p1,&end,10);if(!end||*end){bad=true;break;}
  long v=std::strtol(p2,&end,10);if(!end||*end){bad=true;break;}
  if(b<0||b>=66||c<1||c>150||v<1||v>180||!*p3){bad=true;break;}
  std::string content(p3);while(!content.empty()&&(content.back()=='\n'||content.back()=='\r'))content.pop_back();
  for(char& letter:content)if(letter=='\t')letter=' ';
  if(content.empty()||content.size()>15000){bad=true;break;}
  auto& chapters=data_[(size_t)b];
  if(chapters.size()<=(size_t)c)chapters.resize((size_t)c+1);
  auto& verses=chapters[(size_t)c];
  if(!verses.empty()&&verses.back().number>=v){bad=true;break;}
  verses.push_back({(int)b,(int)c,(int)v,std::move(content)});
  if(++rows>50000){bad=true;break;}
 }
 if(std::ferror(f))bad=true;
 std::fclose(f);
 int chapterCount=0;
 for(const auto& book:data_){
  if(book.size()<2){bad=true;break;}
  for(size_t c=1;c<book.size();++c){if(book[c].empty()){bad=true;break;}++chapterCount;}
 }
 if(bad||rows<30000||rows>40000||chapterCount!=1189||
    !verse({0,1,1})||!verse({65,22,21})){
  error="Invalid/incomplete offline Bible (requires 66 books, 1189 chapters, 30k+ verses)";
  for(auto& x:data_){x.clear();}
  return false;
 }
 count_=rows;error.clear();return true;
}
std::vector<SearchHit> Library::search(std::string query,size_t limit) const {
 std::vector<SearchHit> found;query=lower(query);
 if(query.size()<2||query.size()>70)return found;
 limit=std::min((size_t)400,limit);
 for(int b=0;b<66;b++)for(int c=1;c<=chapterCount(b);c++)
  for(const auto& v:chapter(b,c)){
   if(lower(v.text).find(query)!=std::string::npos){
    found.push_back({{b,c,v.number},v.text});if(found.size()>=limit)return found;
   }
  }
 return found;
}
bool same(Ref a,Ref b){return a.book==b.book&&a.chapter==b.chapter&&a.verse==b.verse;}
bool bookmarked(const Settings& s,Ref r){
 for(Ref x:s.bookmarks){if(same(x,r))return true;}
 return false;
}
void toggle(Settings& s,Ref r){
 auto it=std::find_if(s.bookmarks.begin(),s.bookmarks.end(),[&](Ref x){return same(x,r);});
 if(it!=s.bookmarks.end())s.bookmarks.erase(it);
 else if(s.bookmarks.size()<512)s.bookmarks.push_back(r);
}
int voiceStyleAfterLanguageChange(int previousLanguage,int nextLanguage,int currentStyle){
 // Legacy v0.1.8 forced voice_style=0 when the retro preset was selected.
 // Restore neural speech on exit, including after loading those saved states.
 if(previousLanguage==4 && (nextLanguage==5||(nextLanguage>=0&&nextLanguage<4)))return 1;
 // Do not destroy the user's voice choice when entering retro or when
 // switching among the four normal languages.
 return std::clamp(currentStyle,0,1);
}
bool readSettings(const std::string& path,Settings& s){
 FILE* f=std::fopen(path.c_str(),"rb");if(!f)return false;
 Settings next;char row[256];bool valid=false;
 if(std::fgets(row,sizeof(row),f)&&std::strncmp(row,"BIBLE_STATE=1",13)==0)valid=true;
 while(valid&&std::fgets(row,sizeof(row),f)){
  int a=0,b=0,c=0,d=0,e=0,g=0;
  if(std::sscanf(row,"position=%d,%d,%d",&a,&b,&c)==3){
   if(a>=0&&a<66&&b>=1&&b<=150&&c>=1&&c<=180){
    next.book=a;next.chapter=b;next.selectedVerse=c;
   }
  }else if(std::sscanf(row,"appearance=%d,%d,%d",&a,&b,&c)==3){
   if(a>=22&&a<=38)next.fontSize=a;
   if(b>=0&&b<=1)next.theme=b;
   next.daily=c!=0;
  }else if(std::sscanf(row,"language=%d",&a)==1){
   if(a>=0&&a<=5)next.language=a;
  }else if(std::sscanf(row,"voice_style=%d",&a)==1){
   if(a==0||a==1)next.voiceStyle=a;
  }else if(std::sscanf(row,"bookmark=%d,%d,%d",&d,&e,&g)==3){
   Ref r{d,e,g};
   if(d>=0&&d<66&&e>=1&&e<=150&&g>=1&&g<=180&&!bookmarked(next,r)&&next.bookmarks.size()<512)
    next.bookmarks.push_back(r);
  }
 }
 std::fclose(f);if(valid)s=std::move(next);return valid;
}
bool writeSettings(const std::string& path,const Settings& s){
 const std::string temp=path+".tmp";
 FILE* f=std::fopen(temp.c_str(),"wb");if(!f)return false;
 bool ok=std::fprintf(f,"BIBLE_STATE=1\nposition=%d,%d,%d\nappearance=%d,%d,%d\nlanguage=%d\nvoice_style=%d\n",
 s.book,s.chapter,s.selectedVerse,s.fontSize,s.theme,s.daily?1:0,s.language,s.voiceStyle)>0;
 for(auto r:s.bookmarks)if(std::fprintf(f,"bookmark=%d,%d,%d\n",r.book,r.chapter,r.verse)<0)ok=false;
 if(std::fflush(f)!=0)ok=false;
 if(std::fclose(f)!=0)ok=false;
 if(ok&&std::rename(temp.c_str(),path.c_str())==0)return true;
 std::remove(temp.c_str());return false;
}
Ref dailyReference(int doy){
 static const Ref curated[]={
  {18,46,10},{42,3,16},{0,1,1},{18,23,1},{39,5,9},{45,8,28},
  {22,40,31},{58,1,5},{19,3,5},{42,14,6},{65,21,4},{39,6,33},
  {18,119,105},{43,1,8},{48,4,13},{58,5,7},{18,91,2},{41,1,37},
  {42,8,12},{39,11,28},{61,3,9},{40,10,27},{45,12,12},{18,27,1}
 };
 int n=(int)(sizeof(curated)/sizeof(curated[0]));
 return curated[(unsigned)(doy<1?1:doy)%n];
}
} // namespace bible
