// SPDX-License-Identifier: GPL-3.0-or-later
#include "library_cache.hpp"
#include <zlib.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>
namespace jukebox {
namespace {
constexpr size_t limit=12*1024*1024,maxString=4096;
constexpr uint32_t maxArtists=512,maxAlbums=256,maxTracks=500;
constexpr char magic[8]={'P','S','J','B','L','I','B','3'};
void append32(std::vector<unsigned char>& out,uint32_t value){
 for(unsigned i=0;i<4;i++)out.push_back((unsigned char)((value>>(8*i))&255));
}
void append64(std::vector<unsigned char>& out,uint64_t value){
 append32(out,(uint32_t)value);append32(out,(uint32_t)(value>>32));
}
bool string(std::vector<unsigned char>& out,const std::string& s){
 if(s.size()>maxString)return false;
 append32(out,(uint32_t)s.size());
 out.insert(out.end(),s.begin(),s.end());
 return out.size()<=limit;
}
struct Reader{
 const std::vector<unsigned char>& bytes;
 size_t off=0;
 bool num(uint32_t& result){
  if(bytes.size()-off<4)return false;
  result=0;
  for(unsigned i=0;i<4;i++)result|=(uint32_t)bytes[off++]<<(i*8);
  return true;
 }
 bool num64(uint64_t& result){
  uint32_t lo=0,hi=0;
  if(!num(lo)||!num(hi))return false;
  result=(uint64_t(hi)<<32)|lo;return true;
 }
 bool str(std::string& result){
  uint32_t size=0;
  if(!num(size)||size>maxString||size>bytes.size()-off)return false;
  result.assign((const char*)bytes.data()+off,(size_t)size);off+=size;
  for(char c:result)if(c=='\0'||(unsigned char)c<9)return false;
  return true;
 }
};
bool encode(const Library& lib,std::vector<unsigned char>& out){
 if(lib.artists.empty()||lib.artists.size()>maxArtists)return false;
 append32(out,(uint32_t)lib.artists.size());
 for(const auto& artist:lib.artists){
  if(!string(out,artist.name)||!string(out,artist.path)||
     !string(out,artist.logo)||!string(out,artist.background)||artist.albums.size()>maxAlbums)
   return false;
  append32(out,artist.theme.accent);
  append32(out,artist.theme.secondary);
  append32(out,artist.theme.text);
  append32(out,(uint32_t)artist.albums.size());
  for(const auto& album:artist.albums){
   if(!string(out,album.name)||!string(out,album.path)||
      !string(out,album.cover)||!string(out,album.background)||
      album.tracks.size()>maxTracks)return false;
   append32(out,(uint32_t)album.tracks.size());
   for(const auto& track:album.tracks){
    if(!string(out,track.name)||!string(out,track.path))return false;
    append64(out,track.size);
   }
  }
 }
 return out.size()<=limit;
}
bool decode(const std::vector<unsigned char>& bytes,Library& lib){
 Reader r{bytes};
 uint32_t count=0;
 if(!r.num(count)||count==0||count>maxArtists)return false;
 Library candidate;candidate.artists.reserve(count);
 for(uint32_t i=0;i<count;i++){
  Artist artist;
  if(!r.str(artist.name)||artist.name.empty()||!r.str(artist.path)||
     !r.str(artist.logo)||!r.str(artist.background)||
     !r.num(artist.theme.accent)||!r.num(artist.theme.secondary)||
     !r.num(artist.theme.text))return false;
  uint32_t albums=0;
  if(!r.num(albums)||albums>maxAlbums)return false;
  artist.albums.reserve(albums);
  for(uint32_t j=0;j<albums;j++){
   Album album;
   if(!r.str(album.name)||album.name.empty()||!r.str(album.path)||
      !r.str(album.cover)||!r.str(album.background))return false;
   uint32_t tracks=0;
   if(!r.num(tracks)||tracks>maxTracks)return false;
   album.tracks.reserve(tracks);
   for(uint32_t k=0;k<tracks;k++){
    Track track;
    if(!r.str(track.name)||!r.str(track.path)||track.path.empty()||
       !r.num64(track.size))return false;
    album.tracks.push_back(std::move(track));
   }
   candidate.trackCount+=album.tracks.size();
   artist.albums.push_back(std::move(album));
  }
  candidate.artists.push_back(std::move(artist));
 }
 if(r.off!=bytes.size()||candidate.trackCount==0)return false;
 candidate.report="LIBRARY_CACHE_HIT artists="+std::to_string(candidate.artists.size())+
                  " tracks="+std::to_string(candidate.trackCount);
 lib=std::move(candidate);
 return true;
}
}
std::string library_cache_path(){
#ifdef JUKEBOX_DATA_ROOT
 return std::string(JUKEBOX_DATA_ROOT)+"/library-v3.cache";
#else
 return "/data/PSJukebox/library-v3.cache";
#endif
}
bool save_library_cache(const std::string& path,const Library& library,std::string& error){
 error.clear();
 std::vector<unsigned char> payload;payload.reserve(64*1024);
 if(!encode(library,payload)){error="Cache data exceeds safety limits";return false;}
 const std::string temp=path+".tmp";
 FILE* f=fopen(temp.c_str(),"wb");
 if(!f){error="Cannot open library cache for writing";return false;}
 unsigned char header[16];
 std::memcpy(header,magic,8);
 const uint32_t length=(uint32_t)payload.size();
 const uint32_t checksum=(uint32_t)crc32(0,(const Bytef*)payload.data(),(uInt)payload.size());
 for(unsigned i=0;i<4;i++){
  header[8+i]=(unsigned char)(length>>(8*i));
  header[12+i]=(unsigned char)(checksum>>(8*i));
 }
 bool ok=fwrite(header,1,sizeof(header),f)==sizeof(header)&&
         fwrite(payload.data(),1,payload.size(),f)==payload.size();
 if(fflush(f)!=0)ok=false;
 if(fclose(f)!=0)ok=false;
 if(!ok||rename(temp.c_str(),path.c_str())!=0){
  remove(temp.c_str());error="Cannot finalize library cache";return false;
 }
 return true;
}
bool load_library_cache(const std::string& path,Library& library,std::string& error){
 error.clear();
 FILE* f=fopen(path.c_str(),"rb");
 if(!f){error="No saved library cache";return false;}
 unsigned char h[16];
 bool ok=fread(h,1,sizeof(h),f)==sizeof(h)&&std::memcmp(h,magic,8)==0;
 uint32_t length=0,checksum=0;
 if(ok){
  for(unsigned i=0;i<4;i++){
   length|=(uint32_t)h[8+i]<<(8*i);
   checksum|=(uint32_t)h[12+i]<<(8*i);
  }
  if(length<4||length>limit)ok=false;
 }
 std::vector<unsigned char> payload;
 if(ok){
  payload.resize(length);
  ok=fread(payload.data(),1,payload.size(),f)==payload.size()&&fgetc(f)==EOF;
 }
 fclose(f);
 if(ok)ok=(uint32_t)crc32(0,(const Bytef*)payload.data(),(uInt)payload.size())==checksum;
 if(ok)ok=decode(payload,library);
 if(!ok){error="Invalid/incomplete library cache; using background scan";return false;}
 return true;
}
}
