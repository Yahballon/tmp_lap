// SPDX-License-Identifier: GPL-3.0-or-later
#include "favorites.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <vector>
namespace jukebox {
namespace {
constexpr size_t kMaxKeys=4096,kMaxKeyLength=700,kMaxFile=3*1024*1024;
bool safe(const std::string& key){
 if(key.empty()||key.size()>kMaxKeyLength||key[0]=='/')return false;
 if(key.find('\n')!=std::string::npos||key.find('\r')!=std::string::npos)return false;
 size_t pos=0;
 while(pos<key.size()){
  auto end=key.find('/',pos);
  if(end==std::string::npos)end=key.size();
  if(end==pos||key.compare(pos,end-pos,".")==0||key.compare(pos,end-pos,"..")==0)return false;
  pos=end+1;
 }
 return true;
}
std::string escape_json(const std::string& s){
 std::string result;
 for(char ch:s){
  if(ch=='"'||ch=='\\')result+='\\';
  if(static_cast<unsigned char>(ch)<32)continue;
  result+=ch;
 }
 return result;
}
bool parse_json(const std::string& s,std::set<std::string>& out){
 // Owned fixed-shape schema: {"favorites":["Artist/Album/track.mp3",...]}
 auto k=s.find("\"favorites\"");
 if(k==std::string::npos)return false;
 auto start=s.find('[',k+11),end=s.find(']',start);
 if(start==std::string::npos||end==std::string::npos)return false;
 for(size_t p=start+1;p<end;){
  while(p<end&&(s[p]==' '||s[p]=='\r'||s[p]=='\n'||s[p]==','))p++;
  if(p==end)break;
  if(s[p++]!='"')return false;
  std::string key;
  bool terminated=false;
  for(;p<end;p++){
   const char ch=s[p];
   if(ch=='"'){p++;terminated=true;break;}
   if(ch=='\\'){
    if(++p>=end)return false;
    if(s[p]!='"'&&s[p]!='\\')return false;
   }
   key+=s[p];
   if(key.size()>kMaxKeyLength)return false;
  }
  if(!terminated||!safe(key))return false;
  out.insert(std::move(key));
  if(out.size()>kMaxKeys)return false;
  while(p<end&&(s[p]==' '||s[p]=='\r'||s[p]=='\n'))p++;
  if(p<end&&s[p]!=',')return false;
 }
 return true;
}
}
std::string favorite_key(const std::string& path){
 std::string result;
 if(path.rfind("smbj://",0)==0){result=path.substr(7);}
 else if(path.rfind("jbox://",0)==0){
  auto sep=path.find('/',7);
  if(sep!=std::string::npos)result=path.substr(sep+1);
 }else{
  auto pos=path.find("/Music/");
  if(pos!=std::string::npos)result=path.substr(pos+7);
 }
 return safe(result)?result:std::string{};
}
Favorites::Favorites(std::string filename):filename_(std::move(filename)){}
bool Favorites::load(std::string& error){
 error.clear();keys_.clear();
 FILE* f=fopen(filename_.c_str(),"rb");
 if(!f){if(errno==ENOENT)return true;error="Cannot open favorites: "+std::to_string(errno);return false;}
 if(fseek(f,0,SEEK_END)!=0){fclose(f);error="Favorites seek failed";return false;}
 auto size=ftell(f);
 if(size<0||static_cast<size_t>(size)>kMaxFile){fclose(f);error="Favorites file too large";return false;}
 rewind(f);
 std::string data(static_cast<size_t>(size),0);
 const auto count=fread(data.data(),1,data.size(),f);
 fclose(f);
 if(count!=data.size()){error="Favorites read failed";return false;}
 if(!parse_json(data,keys_)){error="Favorites JSON invalid; existing file preserved";keys_.clear();return false;}
 return true;
}
bool Favorites::contains(const std::string& path) const{
 const auto key=favorite_key(path);
 return !key.empty()&&keys_.find(key)!=keys_.end();
}
bool Favorites::persist(std::string& error) const{
 const auto tmp=filename_+".tmp";
 FILE* f=fopen(tmp.c_str(),"wb");
 if(!f){error="Favorites save failed: "+std::to_string(errno);return false;}
 bool ok=std::fputs("{\"favorites\":[\n",f)>=0;
 size_t i=0;
 for(const auto& key:keys_){
  std::string line="  \""+escape_json(key)+"\""+(++i<keys_.size()?",":"")+"\n";
  if(ok&&std::fwrite(line.data(),1,line.size(),f)!=line.size())ok=false;
 }
 if(ok&&std::fputs("]}\n",f)<0)ok=false;
 if(std::fflush(f)!=0)ok=false;
 if(std::fclose(f)!=0)ok=false;
 if(!ok){std::remove(tmp.c_str());error="Cannot write favorites file";return false;}
 if(std::rename(tmp.c_str(),filename_.c_str())!=0){
  std::remove(tmp.c_str());error="Cannot replace favorites: "+std::to_string(errno);return false;
 }
 return true;
}
bool Favorites::toggle(const std::string& path,bool& selected,std::string& error){
 error.clear();selected=false;
 const std::string key=favorite_key(path);
 if(key.empty()){error="Invalid track path";return false;}
 const auto it=keys_.find(key);
 if(it!=keys_.end()){keys_.erase(it);selected=false;}
 else{
  if(keys_.size()>=kMaxKeys){error="Favorites limit reached";return false;}
  keys_.insert(key);selected=true;
 }
 if(persist(error))return true;
 if(selected)keys_.erase(key);else keys_.insert(key);
 selected=!selected;
 return false;
}
}
