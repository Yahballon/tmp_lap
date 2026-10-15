// SPDX-License-Identifier: GPL-3.0-or-later
// Bounded read-only comic archive implementation using libarchive (RAR4/RAR5/ZIP).
#include "comic_archive.hpp"
#include <archive.h>
#include <archive_entry.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <sys/stat.h>
namespace comic {
namespace {
constexpr size_t kMaxPages=1800,kMaxImage=20*1024*1024;
std::string lower(std::string s) {
 for(char& c:s)c=(char)std::tolower((unsigned char)c);
 return s;
}
bool ends(const std::string& str,const char* ext){
 std::string s=lower(str);
 size_t n=std::strlen(ext);
 return s.size()>=n&&s.compare(s.size()-n,n,ext)==0;
}
bool image_file(const std::string& name) {
 return ends(name,".jpg")||ends(name,".jpeg")||ends(name,".png")||
        ends(name,".gif");
}
std::string normalized_name(std::string path){
 for(char& c:path)if(c=='\\')c='/';
 while(path.compare(0,2,"./")==0)path.erase(0,2);
 return path;
}
bool safe(const std::string& path) {
 if(path.empty()||path.size()>900||path.front()=='/'||path.front()=='\\')return false;
 size_t pos=0;
 while(pos<path.size()){
  size_t end=path.find('/',pos);if(end==std::string::npos)end=path.size();
  if(pos==end||path.substr(pos,end-pos)==".."||path.substr(pos,end-pos)==".")return false;
  for(size_t i=pos;i<end;i++)if(path[i]=='\\'||(unsigned char)path[i]<32)return false;
  pos=end+1;
 }
 return true;
}
bool usable_image_entry(archive_entry* e){
 const mode_t type=archive_entry_filetype(e);
 if(type!=AE_IFREG && type!=0)return false;
 if(archive_entry_is_encrypted(e)>0)return false;
 const auto size=archive_entry_size(e);
 return archive_entry_size_is_set(e)==0 ||
        (size>0 && size<=(la_int64_t)kMaxImage);
}
bool natural_less(const std::string& aa,const std::string& bb){
 const auto a=lower(aa),b=lower(bb);
 size_t i=0,j=0;
 while(i<a.size()&&j<b.size()){
   const bool digitA=std::isdigit((unsigned char)a[i]);
   const bool digitB=std::isdigit((unsigned char)b[j]);
   if(digitA&&digitB){
     size_t ia=i, jb=j;
     while(i<a.size()&&a[i]=='0')++i;
     while(j<b.size()&&b[j]=='0')++j;
     const size_t sa=i,sb=j;
     while(i<a.size()&&std::isdigit((unsigned char)a[i]))++i;
     while(j<b.size()&&std::isdigit((unsigned char)b[j]))++j;
     if(i-sa!=j-sb)return i-sa<j-sb;
     const int cmp=a.compare(sa,i-sa,b,sb,j-sb);
     if(cmp!=0)return cmp<0;
     while(i<a.size()&&std::isdigit((unsigned char)a[i]))++i;
     while(j<b.size()&&std::isdigit((unsigned char)b[j]))++j;
     (void)ia;(void)jb;
     continue;
   }
   if(a[i]!=b[j])return a[i]<b[j];
   ++i;++j;
 }
 return a.size()<b.size();
}
archive* reader(const std::string& filename,std::string& error){
 archive* a=archive_read_new();
 if(!a){error="Unable to allocate archive reader";return nullptr;}
 archive_read_support_filter_none(a);
 archive_read_support_format_rar(a);
 archive_read_support_format_rar5(a);
 archive_read_support_format_zip(a);
 if(archive_read_open_filename(a,filename.c_str(),64*1024)!=ARCHIVE_OK){
  error=archive_error_string(a)?archive_error_string(a):"Cannot open comic archive";
  archive_read_free(a);return nullptr;
 }
 const int type=archive_format(a);
 (void)type; // format may only be established on the first header.
 return a;
}
}
bool is_comic(const std::string& path){return ends(path,".cbr")||ends(path,".cbz");}
bool open(const std::string& path,Book& book){
 book=Book{};book.path=path;
 if(!is_comic(path)){book.error="Not a CBR/CBZ archive";return false;}
 auto* a=reader(path,book.error);if(!a)return false;
 archive_entry* e=nullptr;int rc=ARCHIVE_OK;
 size_t visited=0;
 while((rc=archive_read_next_header(a,&e))==ARCHIVE_OK && visited++<12000){
  const char* raw=archive_entry_pathname_utf8(e);
  if(!raw)raw=archive_entry_pathname(e);
  if(raw){
   std::string name=normalized_name(raw);
   if(safe(name) && image_file(name) && usable_image_entry(e) &&
      book.pages.size()<kMaxPages)
     book.pages.push_back(std::move(name));
  }
  // libarchive handles the state transition to the next RAR entry. A
  // negative data-skip status can occur for unsupported RAR filters;
  // report it rather than returning an apparently empty comic.
  const int skip=archive_read_data_skip(a);
  if(skip<ARCHIVE_WARN){
    book.error=archive_error_string(a)?archive_error_string(a):
               "RAR data cannot be skipped (unsupported compression)";
    archive_read_close(a);archive_read_free(a);
    return false;
  }
 }
 if(rc!=ARCHIVE_EOF){
  book.error=archive_error_string(a)?archive_error_string(a):"Cannot list comic pages";
 }
 archive_read_close(a);archive_read_free(a);
 if(rc!=ARCHIVE_EOF)return false;
 std::sort(book.pages.begin(),book.pages.end(),natural_less);
 book.pages.erase(std::unique(book.pages.begin(),book.pages.end()),book.pages.end());
 if(book.pages.empty()){
  book.error="CBR/CBZ contains no supported JPG, PNG or GIF images; check RAR encryption and archive integrity";
  return false;
 }
 return true;
}
bool extract(const Book& book,size_t index,std::string& encoded,std::string& error){
 encoded.clear();
 if(index>=book.pages.size()){error="Comic page is out of range";return false;}
 archive* a=reader(book.path,error);if(!a)return false;
 archive_entry* e=nullptr;
 bool found=false;int rc=ARCHIVE_OK;
 while((rc=archive_read_next_header(a,&e))==ARCHIVE_OK){
  const char* name=archive_entry_pathname_utf8(e);
  if(!name)name=archive_entry_pathname(e);
  if(name && book.pages[index]==normalized_name(name) &&
      usable_image_entry(e)){
    found=true;
    char chunk[16384];la_ssize_t n=0;
    while((n=archive_read_data(a,chunk,sizeof(chunk)))>0){
      if((size_t)n>kMaxImage-encoded.size()){error="Comic page exceeds 20 MiB";found=false;break;}
      encoded.append(chunk,(size_t)n);
    }
    if(n<0){error=archive_error_string(a)?archive_error_string(a):"RAR image decode failure";found=false;}
    if(found && encoded.empty()){error="CBR image decompressed to zero bytes";found=false;}
    break;
  }
  archive_read_data_skip(a);
 }
 if(!found && error.empty()){
   const char* detail=archive_error_string(a);
   error=detail && *detail ? detail : "Comic page could not be decoded";
 }
 if(found){
   // Ensure the payload is actually an image and not an unsupported or
   // renamed file. RAR5 requires successful data decoding, not just a header.
   const auto starts=[&](const char* signature,size_t n){
     return encoded.size()>=n && std::memcmp(encoded.data(),signature,n)==0;
   };
   const bool valid=starts("\\xFF\\xD8",2) ||
      starts("\\x89PNG\\r\\n\\x1a\\n",8) ||
      starts("GIF87a",6) || starts("GIF89a",6);
   if(!valid){
     error="CBR page is not a decoded JPG, PNG or GIF image";
     found=false;encoded.clear();
   }
 }
 archive_read_close(a);archive_read_free(a);
 return found&&!encoded.empty();
}
bool cover(const std::string& path,std::string& img){
 Book b;if(!open(path,b))return false;
 std::string error;return extract(b,0,img,error);
}
} // namespace comic
