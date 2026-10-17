// SPDX-License-Identifier: GPL-3.0-or-later
#include "library.hpp"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <map>
#include <set>
#include <sstream>
#include <sys/stat.h>

namespace jukebox {
namespace {
std::string lowercase(std::string value) {
 for(char& c:value)c=(char)std::tolower((unsigned char)c);
 return value;
}
bool safe_name(const std::string& s) {
 return !s.empty()&&s!="."&&s!=".."&&s[0]!='.'&&
   s.find('/')==std::string::npos&&s.find('\\')==std::string::npos;
}
bool is_dir(const std::string& path) {
 struct stat st{};
 if(lstat(path.c_str(),&st)==0)return S_ISDIR(st.st_mode);
 if(errno!=EPERM&&errno!=ENOSYS)return false;
 DIR* d=opendir(path.c_str());
 if(!d)return false;
 closedir(d);
 return true;
}
bool is_file(const std::string& path,uint64_t* size=nullptr) {
 struct stat st{};
 if(lstat(path.c_str(),&st)==0){
  if(!S_ISREG(st.st_mode))return false;
  if(size)*size=(uint64_t)st.st_size;
  return true;
 }
 if(errno!=EPERM&&errno!=ENOSYS)return false;
 // The PS5 app sandbox may block lstat on its own imported music,
 // even though the file can be opened for playback.
 FILE* f=fopen(path.c_str(),"rb");
 if(!f)return false;
 uint64_t len=0;
 if(fstat(fileno(f),&st)==0){
  if(!S_ISREG(st.st_mode)){fclose(f);return false;}
  len=(uint64_t)st.st_size;
 }else{
  if(fseek(f,0,SEEK_END)!=0){fclose(f);return false;}
  const long end=ftell(f);
  if(end<0){fclose(f);return false;}
  len=(uint64_t)end;
 }
 fclose(f);
 if(size)*size=len;
 return true;
}
std::vector<std::string> directories(const std::string& path) {
 std::vector<std::string> result;
 DIR* dir=opendir(path.c_str());
 if(!dir)return result;
 while(dirent* e=readdir(dir)) {
  const std::string name=e->d_name;
  if(safe_name(name)&&is_dir(path+"/"+name))result.push_back(name);
 }
 closedir(dir);
 std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){
  return lowercase(a)<lowercase(b);
 });
 return result;
}
std::string art(const std::string& folder,const std::string& stem) {
 // Fixed filenames: PNG takes priority, then JPEG; comparisons are case insensitive.
 DIR* dir=opendir(folder.c_str());if(!dir)return {};
 std::string chosen;int rank=99;
 const std::string png=lowercase(stem)+".png";
 const std::string jpg=lowercase(stem)+".jpg";
 const std::string jpeg=lowercase(stem)+".jpeg";
 while(dirent* e=readdir(dir)) {
  const std::string name=e->d_name,lower=lowercase(name);
  int preference=lower==png?0:(lower==jpg?1:(lower==jpeg?2:99));
  const std::string path=folder+"/"+name;
  uint64_t size=0;
  if(preference<rank&&is_file(path,&size)&&size>8&&size<=12u*1024u*1024u) {
   rank=preference;chosen=path;
  }
 }
 closedir(dir);return chosen;
}
uint32_t parse_hex(std::string raw,uint32_t fallback) {
 raw.erase(std::remove_if(raw.begin(),raw.end(),[](unsigned char c){return std::isspace(c);}),raw.end());
 if(raw.size()==7&&raw[0]=='#')raw.erase(raw.begin());
 if(raw.size()!=6)return fallback;
 for(char c:raw)if(!std::isxdigit((unsigned char)c))return fallback;
 return (uint32_t)strtoul(raw.c_str(),nullptr,16);
}
std::string trim(std::string s) {
 auto begin=s.find_first_not_of(" \t\r\n");
 if(begin==std::string::npos)return {};
 auto end=s.find_last_not_of(" \t\r\n");
 return s.substr(begin,end-begin+1);
}
std::vector<Track> tracks(const std::string& folder) {
 std::vector<Track> list;
 DIR* dir=opendir(folder.c_str());if(!dir)return list;
 while(dirent* e=readdir(dir)) {
  const std::string name=e->d_name;
  if(!safe_name(name)||!is_music(name))continue;
  uint64_t size=0;
  const std::string path=folder+"/"+name;
  if(!is_file(path,&size)||size<1024||size>8ull*1024*1024*1024)continue;
  auto dot=name.find_last_of('.');
  std::string title=name.substr(0,dot);
  for(char& c:title)if(c=='_')c=' ';
  list.push_back({title,path,size});
  if(list.size()>=400)break;
 }
 closedir(dir);
 std::sort(list.begin(),list.end(),[](const Track& a,const Track& b){
  return lowercase(a.name)<lowercase(b.name);
 });
 return list;
}
}
bool is_music(const std::string& filename) {
 const auto dot=filename.find_last_of('.');
 if(dot==std::string::npos)return false;
 const auto ext=lowercase(filename.substr(dot));
 return ext==".mp3"||ext==".flac"||ext==".m4a"||ext==".aac"||
        ext==".wav"||ext==".ogg"||ext==".opus"||ext==".wma";
}
Theme parse_theme_text(const std::string& data) {
 Theme t;
 std::istringstream stream(data);
 std::string line;
 for(int i=0;i<32 && std::getline(stream,line);i++){
  line=trim(line);
  if(line.empty()||line[0]=='#'||line[0]==';')continue;
  const auto eq=line.find('=');
  if(eq==std::string::npos)continue;
  const auto key=lowercase(trim(line.substr(0,eq)));
  const auto value=trim(line.substr(eq+1));
  if(key=="accent")t.accent=parse_hex(value,t.accent);
  if(key=="secondary")t.secondary=parse_hex(value,t.secondary);
  if(key=="text")t.text=parse_hex(value,t.text);
 }
 return t;
}
Theme read_theme(const std::string& artistFolder) {
 FILE* f=fopen((artistFolder+"/theme.ini").c_str(),"rb");
 if(!f)return {};
 std::string content;char bytes[512];
 for(size_t n=0;(n=fread(bytes,1,sizeof(bytes),f))>0&&content.size()<4096;)
  content.append(bytes,std::min<size_t>(n,4096-content.size()));
 fclose(f);
 return parse_theme_text(content);
}
std::vector<std::string> music_roots() {
 return {"/data/PSJukebox/Music","/download0/PSJukebox/Music",
         "/mnt/usb0/PSJukebox/Music","/mnt/usb1/PSJukebox/Music",
         "/mnt/usb2/PSJukebox/Music","/mnt/usb3/PSJukebox/Music",
         "/mnt/usb0/Music","/mnt/usb1/Music"};
}
std::string album_background(const Artist& artist,const Album& album) {
 return !album.background.empty()?album.background:artist.background;
}
Library scan(const std::vector<std::string>& roots) {
 Library out;std::map<std::string,size_t> artistIndexes;std::set<std::string> visited;
 for(const auto& root:roots) {
  if(root.empty()||!visited.insert(root).second)continue;
  if(!is_dir(root)) {
   out.report+="UNAVAILABLE "+root+" errno="+std::to_string(errno)+"; ";
   continue;
  }
  unsigned found=0;
  for(const auto& name:directories(root)) {
   if(out.artists.size()>=256)break;
   const std::string artistDir=root+"/"+name;
   const auto albumNames=directories(artistDir);
   // Empty artist folders are still valid library entries.
   const auto key=lowercase(name);
   size_t index=0;
   auto it=artistIndexes.find(key);
   if(it==artistIndexes.end()) {
    index=out.artists.size();artistIndexes[key]=index;
    Artist artist;artist.name=name;artist.path=artistDir;
    artist.logo=art(artistDir,"logo");
    artist.background=art(artistDir,"background");
    artist.theme=read_theme(artistDir);
    out.artists.push_back(std::move(artist));
   } else index=it->second;
   Artist& artist=out.artists[index];
   // Artist root can contain loose tracks; expose as a virtual Singles album.
   auto loose=tracks(artistDir);
   if(!loose.empty()){
    Album singles;singles.name="Singles";singles.path=artistDir;
    singles.cover=art(artistDir,"cover");
    singles.background=art(artistDir,"background");
    singles.tracks=std::move(loose);
    out.trackCount+=singles.tracks.size();
    artist.albums.push_back(std::move(singles));
    found++;
   }
   for(const auto& albumName:albumNames) {
    if(artist.albums.size()>=128)break;
    const std::string albumDir=artistDir+"/"+albumName;
    Album album;album.name=albumName;album.path=albumDir;
    album.tracks=tracks(albumDir);
    // Keep albums without recognized songs visible for diagnosis.
    album.cover=art(albumDir,"cover");
    album.background=art(albumDir,"background");
    out.trackCount+=album.tracks.size();
    artist.albums.push_back(std::move(album));
    found++;
   }
  }
  out.report+="ROOT "+root+" albums="+std::to_string(found)+"; ";
 }
 for(auto& a:out.artists) {
  std::sort(a.albums.begin(),a.albums.end(),[](const auto& x,const auto& y){
   return lowercase(x.name)<lowercase(y.name);
  });
 }
 std::sort(out.artists.begin(),out.artists.end(),[](const auto& a,const auto& b){
  return lowercase(a.name)<lowercase(b.name);
 });
 // Keep all visible artist folders even if their album folders have no playable tracks.
 for(const auto& a:out.artists){
  size_t count=0;
  for(const auto& album:a.albums)count+=album.tracks.size();
  out.report+="ARTIST="+a.name+" ALBUMS="+std::to_string(a.albums.size())+
              " TRACKS="+std::to_string(count)+"; ";
 }
 out.report+="ARTISTS="+std::to_string(out.artists.size())+
            " TRACKS="+std::to_string(out.trackCount);
 return out;
}
std::vector<PackedAlbum> find_packed_albums(const std::vector<std::string>& roots){
 std::vector<PackedAlbum> result;
 for(const auto& root:roots){
  // Drop <Album>.album straight in Music/: use <Artist>.artist inside it.
  DIR* base=opendir(root.c_str());
  if(base){
   while(dirent* e=readdir(base)){
    const std::string name=e->d_name,low=lowercase(name);
    if(!safe_name(name)||low.size()<=6||low.substr(low.size()-6)!=".album")continue;
    uint64_t size=0;
    const std::string full=root+"/"+name;
    if(is_file(full,&size)&&size>=22&&size<=1024ULL*1024*1024)
     result.push_back({"",full,size});
   }
   closedir(base);
  }
  for(const auto& name:directories(root)){
   const auto folder=root+"/"+name;
   DIR* d=opendir(folder.c_str());if(!d)continue;
   while(dirent* e=readdir(d)){
    const std::string filename=e->d_name,low=lowercase(filename);
    if(!safe_name(filename)||low.size()<=6||low.substr(low.size()-6)!=".album")continue;
    uint64_t size=0;
    const std::string path=folder+"/"+filename;
    if(is_file(path,&size)&&size>=22&&size<=1024ULL*1024*1024)
     result.push_back({name,path,size});
   }
   closedir(d);
  }
 }
 return result;
}
// Unify direct and helper-discovered artists rather than showing duplicates.
void merge(Library& target,Library&& other){
 for(auto& added:other.artists){
  auto artist=std::find_if(target.artists.begin(),target.artists.end(),
   [&](const Artist& a){return lowercase(a.name)==lowercase(added.name);});
  if(artist==target.artists.end()){
   target.artists.push_back(std::move(added));continue;
  }
  if(artist->logo.empty())artist->logo=added.logo;
  if(artist->background.empty())artist->background=added.background;
  if(artist->theme.accent==Theme{}.accent)artist->theme=added.theme;
  for(auto& album:added.albums){
   auto match=std::find_if(artist->albums.begin(),artist->albums.end(),
    [&](const Album& a){return lowercase(a.name)==lowercase(album.name);});
   if(match==artist->albums.end()){
    artist->albums.push_back(std::move(album));continue;
   }
   if(match->cover.empty())match->cover=album.cover;
   if(match->background.empty())match->background=album.background;
   for(auto& song:album.tracks){
    auto dup=std::find_if(match->tracks.begin(),match->tracks.end(),[&](const Track& t){
      return lowercase(t.name)==lowercase(song.name) && t.size==song.size;
    });
    if(dup==match->tracks.end())match->tracks.push_back(std::move(song));
   }
   std::sort(match->tracks.begin(),match->tracks.end(),[](const Track& a,const Track& b){
     return lowercase(a.name)<lowercase(b.name);
   });
  }
  std::sort(artist->albums.begin(),artist->albums.end(),[](const Album& a,const Album& b){
    return lowercase(a.name)<lowercase(b.name);
  });
 }
 std::sort(target.artists.begin(),target.artists.end(),[](const Artist& a,const Artist& b){
  return lowercase(a.name)<lowercase(b.name);
 });
 target.trackCount=0;
 for(const auto& a:target.artists)
  for(const auto& b:a.albums)target.trackCount+=b.tracks.size();
 target.report+=" | "+other.report;
}
} // namespace jukebox
