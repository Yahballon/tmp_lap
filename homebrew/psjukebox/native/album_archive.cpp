// SPDX-License-Identifier: GPL-3.0-or-later
#include "album_archive.hpp"
#include "library.hpp"
#include <zlib.h>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
namespace jukebox {
namespace {
constexpr uint64_t maxPack=1024ULL*1024*1024; // 1 GiB compressed file
constexpr uint64_t maxExpanded=2ULL*1024*1024*1024;
constexpr uint64_t maxTrack=512ULL*1024*1024;
constexpr uint64_t maxArt=12ULL*1024*1024;
struct Entry{
 std::string name;
 uint32_t crc=0,compressed=0,plain=0,local=0;
 uint16_t method=0,flags=0;
};
uint16_t u16(const unsigned char* p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
uint32_t u32(const unsigned char* p){return uint32_t(u16(p))|(uint32_t(u16(p+2))<<16);}
std::string lower(std::string s){
 for(char& c:s)c=(char)std::tolower((unsigned char)c);
 return s;
}
bool safe(const std::string& s){
 if(s.empty()||s.size()>180||s=="."||s==".."||s[0]=='.')return false;
 for(unsigned char c:s)if(c<32||c==127||c=='/'||c=='\\')return false;
 return true;
}
bool seek(FILE* f,uint64_t pos){return pos<=LONG_MAX&&fseek(f,(long)pos,SEEK_SET)==0;}
bool exact(FILE* f,void* p,size_t count){return fread(p,1,count,f)==count;}
bool dir(const std::string& p){
 struct stat st{};
 if(lstat(p.c_str(),&st)==0)return S_ISDIR(st.st_mode);
 const int failure=errno;
 // On PS5, lstat can fail with EPERM in an otherwise writable/cacheable
 // directory. stat and opendir can still work for app-owned cache paths.
 if((failure==EPERM||failure==ENOSYS)&&stat(p.c_str(),&st)==0)
  return S_ISDIR(st.st_mode);
 if(mkdir(p.c_str(),0755)==0)return true;
 if(errno==EEXIST){
  DIR* d=opendir(p.c_str());
  if(d){closedir(d);return true;}
 }
 return false;
}
FILE* open_album(const std::string& path,uint64_t& size,std::string& error){
 size=0;
 int flags=O_RDONLY;
#ifdef O_NOFOLLOW
 flags|=O_NOFOLLOW;  // Never allow symlink traversal from an album reference.
#endif
 int fd=open(path.c_str(),flags);
 if(fd<0){error="Cannot open packed album errno="+std::to_string(errno);return nullptr;}
 struct stat st{};
 if(fstat(fd,&st)==0){
  if(!S_ISREG(st.st_mode)||st.st_size<22||uint64_t(st.st_size)>maxPack){
   error="Packed album is not a regular file or exceeds 1 GiB size";
   close(fd);return nullptr;
  }
  size=(uint64_t)st.st_size;
 }else{
  // Certain PS5 sandbox combinations deny path metadata syscalls; use the
  // open descriptor for sizing rather than lstat(path). Keep O_NOFOLLOW.
  int code=errno;
  if(code!=EPERM&&code!=ENOSYS&&code!=EACCES){
   error="Packed album fstat failed errno="+std::to_string(code);
   close(fd);return nullptr;
  }
  const off_t end=lseek(fd,0,SEEK_END);
  if(end<22||(uint64_t)end>maxPack||lseek(fd,0,SEEK_SET)!=0){
   error="Packed album seek/size failed errno="+std::to_string(errno);
   close(fd);return nullptr;
  }
  size=(uint64_t)end;
 }
 FILE* f=fdopen(fd,"rb");
 if(!f){error="Packed album stream failed errno="+std::to_string(errno);close(fd);}
 return f;
}
uint64_t hash(const std::string& path){
 uint64_t h=14695981039346656037ULL;
 for(unsigned char c:path){h^=c;h*=1099511628211ULL;}
 return h;
}
std::string hex(uint64_t n){
 char b[20];snprintf(b,sizeof(b),"%016llx",(unsigned long long)n);return b;
}
bool wanted(const std::string& name){
 if(is_music(name))return true;
 const std::string s=lower(name);
 return s=="cover.png"||s=="cover.jpg"||s=="cover.jpeg"||
        s=="background.png"||s=="background.jpg"||s=="background.jpeg";
}
bool write_entry(FILE* zip,const Entry& e,const std::string& destination,
                 uint64_t limit,std::string& error){
 unsigned char lh[30]{};
 if(!seek(zip,e.local)||!exact(zip,lh,sizeof(lh))||u32(lh)!=0x04034b50u){
  error="Invalid ZIP local header";return false;
 }
 if(u16(lh+8)!=e.method||u16(lh+6)&1){
  error="ZIP local entry mismatch/encrypted";return false;
 }
 const uint64_t data=e.local+30ULL+u16(lh+26)+u16(lh+28);
 if(!seek(zip,data)){error="Invalid ZIP member offset";return false;}
 FILE* out=fopen((destination+".part").c_str(),"wb");
 if(!out){error="Cannot write album cache: "+std::to_string(errno);return false;}
 bool okay=true;
 uint64_t bytesLeft=e.compressed,plain=0;
 uLong crc=crc32(0L,Z_NULL,0);
 // Heap-owned scratch avoids 64 KiB of local buffers overflowing the
 // limited default stack of a native PS5 library scan worker.
 std::vector<unsigned char> input(32768),output(32768);
 z_stream z{};
 bool deflate=e.method==8,finished=false;
 if(deflate&&inflateInit2(&z,-MAX_WBITS)!=Z_OK)okay=false;
 while(okay&&bytesLeft){
  const size_t n=(size_t)std::min<uint64_t>(bytesLeft,input.size());
  if(!exact(zip,input.data(),n)){okay=false;error="Truncated album member";break;}
  bytesLeft-=n;
  if(!deflate){
   plain+=n;
   if(plain>limit||fwrite(input.data(),1,n,out)!=n){okay=false;error="Album file size/write limit";break;}
   crc=crc32(crc,input.data(),(uInt)n);
   continue;
  }
  z.next_in=input.data();z.avail_in=(uInt)n;
  while(z.avail_in&&okay){
   z.next_out=output.data();z.avail_out=(uInt)output.size();
   const int rc=inflate(&z,Z_NO_FLUSH);
   const size_t got=output.size()-z.avail_out;
   plain+=got;
   if(plain>limit||(got&&fwrite(output.data(),1,got,out)!=got)){
    okay=false;error="Album inflate size/write limit";break;
   }
   if(got)crc=crc32(crc,output.data(),(uInt)got);
   if(rc==Z_STREAM_END){finished=true;break;}
   if(rc!=Z_OK||(got==0&&z.avail_in==0&&bytesLeft==0)){
    okay=false;error="Invalid deflate stream";break;
   }
  }
  if(finished&&bytesLeft){okay=false;error="ZIP compressed size mismatch";}
 }
 if(deflate){inflateEnd(&z);if(!finished)okay=false;}
 if(plain!=e.plain||crc!=e.crc){okay=false;error="ZIP CRC/size mismatch";}
 if(fflush(out)!=0)okay=false;
 if(fclose(out)!=0)okay=false;
 const std::string part=destination+".part";
 if(!okay){remove(part.c_str());return false;}
 if(rename(part.c_str(),destination.c_str())!=0){
  remove(part.c_str());error="Cannot finalize archive file";return false;
 }
 return true;
}
}
bool is_album_file(const std::string& name){
 auto s=lower(name);
 return s.size()>6&&s.substr(s.size()-6)==".album";
}
std::string album_stem(const std::string& filename){
 auto slash=filename.find_last_of("/\\");
 const auto base=slash==std::string::npos?filename:filename.substr(slash+1);
 if(!is_album_file(base))return {};
 const auto stem=base.substr(0,base.size()-6);
 return safe(stem)?stem:std::string{};
}
std::string album_artist_marker(const std::string& archive,std::string& error){
 error.clear();
 uint64_t length=0;
 FILE* f=open_album(archive,length,error);
 if(!f)return {};
 std::string artist;
 do{
  const size_t n=(size_t)std::min<uint64_t>(length,65557);
  std::vector<unsigned char> tail(n);
  if(!seek(f,length-n)||!exact(f,tail.data(),n)){error="ZIP end unavailable";break;}
  size_t end=n;
  for(size_t i=n-22;;i--){if(u32(tail.data()+i)==0x06054b50u){end=i;break;}if(i==0)break;}
  if(end==n||u16(tail.data()+end+4)!=0||u16(tail.data()+end+6)!=0||
     u16(tail.data()+end+8)!=u16(tail.data()+end+10)||
     u16(tail.data()+end+10)>512){error="Invalid ZIP central directory";break;}
  const uint64_t offset=u32(tail.data()+end+16),sz=u32(tail.data()+end+12);
  if(offset+sz>length||!seek(f,offset)){error="Invalid ZIP index";break;}
  bool bad=false;
  for(unsigned i=0;i<u16(tail.data()+end+10);i++){
   unsigned char hd[46]{};
   if(!exact(f,hd,sizeof(hd))||u32(hd)!=0x02014b50u){bad=true;break;}
   const unsigned namelen=u16(hd+28),extra=u16(hd+30),comment=u16(hd+32);
   if(namelen<1||namelen>360){bad=true;break;}
   std::string name(namelen,'\0');
   if(!exact(f,name.data(),name.size())||fseek(f,(long)(extra+comment),SEEK_CUR)!=0){bad=true;break;}
   // A marker may be at archive root or under the same single wrapper
   // folder used by its cover/audio files. The *filename* is the artist.
   const size_t sep=name.find('/');
   if(sep!=std::string::npos){
    if(!safe(name.substr(0,sep))||name.find('/',sep+1)!=std::string::npos)continue;
    name=name.substr(sep+1);
   }
   if(name.size()<=7||lower(name.substr(name.size()-7))!=".artist")continue;
   const auto candidate=name.substr(0,name.size()-7);
   if(!safe(candidate)||u32(hd+24)>4096||u16(hd+8)&1||artist.size()){
    bad=true;break;
   }
   artist=candidate;
  }
  if(bad){error="Invalid or duplicate .artist marker";artist.clear();}
  else if(artist.empty())error="Root .album requires a <Name>.artist marker";
 }while(false);
 fclose(f);
 return artist;
}
std::string archive_cache_root(){
#ifdef JUKEBOX_DATA_ROOT
 return std::string(JUKEBOX_DATA_ROOT)+"/AlbumCache";
#else
 return "/data/PSJukebox/AlbumCache";
#endif
}
bool unpack_album(const std::string& path,const std::string& artist,
                  const std::string& album,const std::string& sourceKey,
                  const std::string& cacheRoot,std::string& musicRoot,
                  std::string& error,Library* imported){
 musicRoot.clear();error.clear();
 if(imported)*imported=Library{};
 if(!safe(artist)||!safe(album)||sourceKey.empty()){error="Invalid album/artist name";return false;}
 uint64_t length=0;
 FILE* z=open_album(path,length,error);
 if(!z)return false;
 // A content-size-qualified cache directory isolates different ZIP files
 // while preserving the original Artist/Album layout for the library scanner.
 const std::string id=hex(hash(sourceKey+"#"+std::to_string(length)));
 const std::string home=cacheRoot+"/"+id;
 musicRoot=home+"/Music";
 const std::string folder=musicRoot+"/"+artist+"/"+album;
 const std::string marker=folder+"/.album-ready";
 // Even an already-imported archive needs an index on consoles where
 // opendir() is denied for the app-owned /data/.../AlbumCache directory.
 // Do not return early for the .album-ready marker.
 FILE* ready=fopen(marker.c_str(),"rb");
 const bool cached=ready!=nullptr;
 if(ready)fclose(ready);
 bool okay=false;
 std::vector<Entry> validatedFiles;
 do{
  const size_t tailSize=(size_t)std::min<uint64_t>(length,65557);
  std::vector<unsigned char> tail(tailSize);
  if(!seek(z,length-tailSize)||!exact(z,tail.data(),tail.size())){error="ZIP tail unavailable";break;}
  size_t eocd=tailSize;
  for(size_t i=tailSize-22;;i--){
   if(u32(tail.data()+i)==0x06054b50u){eocd=i;break;}
   if(i==0)break;
  }
  if(eocd==tailSize){error="ZIP central directory missing";break;}
  const unsigned char* e=tail.data()+eocd;
  if(u16(e+4)!=0||u16(e+6)!=0||u16(e+8)!=u16(e+10)||
     u16(e+10)>512||u16(e+10)==0xffff||u32(e+12)==0xffffffffu||
     u32(e+16)==0xffffffffu){error="Unsupported multi-disk/ZIP64 album";break;}
  const uint64_t center=u32(e+16),end=center+u32(e+12);
  if(end>length||u16(e+20)+eocd+22>tailSize){error="Invalid ZIP directory bounds";break;}
  if(!seek(z,center)){error="Cannot seek ZIP directory";break;}
  std::vector<Entry> files;
  uint64_t expanded=0;
  bool invalid=false;
  for(unsigned i=0;i<u16(e+10);i++){
   unsigned char h[46]{};
   if(!exact(z,h,sizeof(h))||u32(h)!=0x02014b50u){invalid=true;break;}
   const unsigned n=u16(h+28),extra=u16(h+30),comment=u16(h+32);
   if(n==0||n>360||extra>65535||comment>65535){invalid=true;break;}
   std::string name(n,'\0');
   if(!exact(z,name.data(),n)||fseek(z,(long)(extra+comment),SEEK_CUR)!=0){invalid=true;break;}
   const uint32_t compressed=u32(h+20),plain=u32(h+24);
   // Directory/symlink entries are never extracted, and only supported media
   // files are accepted, at archive root or inside ONE wrapper folder.
   const auto slash=name.find('/');
   std::string base=name;
   if(slash!=std::string::npos){
    if(!safe(name.substr(0,slash))||name.find('/',slash+1)!=std::string::npos)continue;
    base=name.substr(slash+1);
   }
   if(!safe(base)||!wanted(base))continue;
   const auto method=u16(h+10),flags=u16(h+8);
   const unsigned unixMode=u32(h+38)>>16;
   if((unixMode&0170000)==0120000){invalid=true;error="Symlink in album";break;}
   if(flags&1||(method!=0&&method!=8)||compressed==0||u32(h+42)>=length||
      plain>(is_music(base)?maxTrack:maxArt)||uint64_t(plain)>maxExpanded-expanded||
      (compressed>0&&uint64_t(plain)>uint64_t(compressed)*1500)){
    invalid=true;error="Unsafe/unsupported ZIP member";break;
   }
   expanded+=plain;
   files.push_back({base,u32(h+16),compressed,plain,u32(h+42),method,flags});
  }
  if(invalid||files.empty()){if(error.empty())error="Invalid or empty packed album";break;}
  unsigned playable=0;
  for(const auto& entry:files)if(is_music(entry.name))++playable;
  if(!playable){error="Packed album has no supported playable tracks";break;}
  if(!cached){
   if(!dir(cacheRoot)||!dir(home)||!dir(musicRoot)||!dir(musicRoot+"/"+artist)||!dir(folder)){
    error="Could not create album cache";break;
   }
   bool extracted=true;
   for(const auto& entry:files){
    if(!write_entry(z,entry,folder+"/"+entry.name,
                    is_music(entry.name)?maxTrack:maxArt,error)){extracted=false;break;}
   }
   if(!extracted)break;
   FILE* done=fopen(marker.c_str(),"wb");
   if(!done){error="Cannot write archive-ready marker";break;}
   fprintf(done,"ZIP %llu entries=%zu\n",(unsigned long long)length,files.size());
   fclose(done);
  }
  validatedFiles=std::move(files);
  okay=true;
 }while(false);
 fclose(z);
 if(!okay)return false;
 // Build the media catalog from the verified ZIP central-directory entries.
 // opendir/stat on our new AlbumCache can fail with EPERM on PS5, even though
 // files created by the app can be opened by FFmpeg, so never rescan the path.
 if(imported){
  Library catalog;
  Artist a;a.name=artist;a.path=musicRoot+"/"+artist;
  Album albumEntry;albumEntry.name=album;albumEntry.path=folder;
  int coverRank=100,backgroundRank=100;
  for(const auto& e:validatedFiles){
   const std::string filename=lower(e.name),filepath=folder+"/"+e.name;
   auto rank=[&](const char* stem)->int{
    const std::string prefix=stem;
    if(filename==prefix+".png")return 0;
    if(filename==prefix+".jpg")return 1;
    if(filename==prefix+".jpeg")return 2;
    return 99;
   };
   if(is_music(e.name)){
    auto dot=e.name.find_last_of('.');
    std::string title=e.name.substr(0,dot);
    for(char& c:title)if(c=='_')c=' ';
    albumEntry.tracks.push_back({std::move(title),filepath,e.plain});
   }
   const int c=rank("cover"),b=rank("background");
   if(c<coverRank){coverRank=c;albumEntry.cover=filepath;}
   if(b<backgroundRank){backgroundRank=b;albumEntry.background=filepath;}
  }
  std::sort(albumEntry.tracks.begin(),albumEntry.tracks.end(),[](const Track& x,const Track& y){
   return lower(x.name)<lower(y.name);
  });
  catalog.trackCount=albumEntry.tracks.size();
  catalog.report="ARCHIVE_INDEX "+artist+"/"+album+" tracks="+std::to_string(catalog.trackCount);
  a.albums.push_back(std::move(albumEntry));
  catalog.artists.push_back(std::move(a));
  *imported=std::move(catalog);
 }
 return true;
}
}
