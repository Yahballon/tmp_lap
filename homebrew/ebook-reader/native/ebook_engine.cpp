// SPDX-License-Identifier: GPL-3.0-or-later
// A bounded, no-browser EPUB spine reader and text-only PDF fallback.
// Complex layouts, SVG, encrypted and image-only PDFs require a separate renderer.
#include "ebook_engine.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <map>
#include <set>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

namespace ebook {
namespace {
constexpr size_t kMaxInput=128*1024*1024, kMaxEntry=12*1024*1024;
std::string lower(std::string s) { for(char& c:s)c=(char)std::tolower((unsigned char)c);return s; }
bool ends(const std::string& s,const char* suffix){
    const size_t n=std::strlen(suffix); return s.size()>=n && lower(s.substr(s.size()-n))==suffix;
}
uint16_t u16(const std::string& d,size_t p){
 return p+2<=d.size()?(uint16_t)((unsigned char)d[p]|((unsigned char)d[p+1]<<8)):0;
}
uint32_t u32(const std::string& d,size_t p){
 return (uint32_t)u16(d,p)|((uint32_t)u16(d,p+2)<<16);
}
bool load(const std::string& path,std::string& dst,size_t cap=kMaxInput){
 FILE* f=std::fopen(path.c_str(),"rb");if(!f)return false;
 if(std::fseek(f,0,SEEK_END)){std::fclose(f);return false;}
 long n=std::ftell(f);if(n<0 || (size_t)n>cap){std::fclose(f);return false;}
 std::rewind(f);dst.resize((size_t)n);
 bool ok=dst.empty()||std::fread(&dst[0],1,dst.size(),f)==dst.size();
 std::fclose(f);return ok;
}
bool inflate_raw(const char* data,size_t n,size_t expected,std::string& out){
 if(expected>kMaxEntry || n>kMaxInput) return false;
 z_stream z{};if(inflateInit2(&z,-MAX_WBITS)!=Z_OK)return false;
 out.clear();out.reserve(std::min<size_t>(expected,65536));
 z.next_in=(Bytef*)data;z.avail_in=(uInt)n;
 char buf[8192];int rc=Z_OK;
 while(rc==Z_OK && out.size()<kMaxEntry){
   z.next_out=(Bytef*)buf;z.avail_out=sizeof(buf);
   rc=inflate(&z,Z_NO_FLUSH);
   const size_t count=sizeof(buf)-z.avail_out;
   if(count>kMaxEntry-out.size()){rc=Z_MEM_ERROR;break;}
   out.append(buf,count);
 }
 inflateEnd(&z);
 return rc==Z_STREAM_END && (expected==0 || expected==out.size());
}
struct ZipEntry{size_t offset,compressed,uncompressed;int method;};
class Zip {
 std::string bytes;
 std::map<std::string,ZipEntry> items;
public:
 bool open(const std::string& path) {
   if(!load(path,bytes))return false;
   if(bytes.size()<22)return false;
   size_t eocd=std::string::npos;
   const size_t min=bytes.size()>65557?bytes.size()-65557:0;
   for(size_t i=bytes.size()-22;;--i){
     if(u32(bytes,i)==0x06054b50){eocd=i;break;}
     if(i==min)break;
   }
   if(eocd==std::string::npos)return false;
   const uint32_t count=u16(bytes,eocd+10),size=u32(bytes,eocd+12),offset=u32(bytes,eocd+16);
   if(count>10000 || (size_t)offset+size>bytes.size())return false;
   size_t p=offset;
   for(uint32_t i=0;i<count;i++){
     if(p+46>bytes.size()||u32(bytes,p)!=0x02014b50)return false;
     const uint16_t flag=u16(bytes,p+8),method=u16(bytes,p+10);
     const uint32_t packed=u32(bytes,p+20),unpacked=u32(bytes,p+24),local=u32(bytes,p+42);
     const size_t nameLen=u16(bytes,p+28),extra=u16(bytes,p+30),comment=u16(bytes,p+32);
     if(p+46+nameLen+extra+comment>bytes.size())return false;
     auto name=bytes.substr(p+46,nameLen);
     // Reject encrypted entries, absolute paths, path traversal and ZIP64.
     if(!(flag&1)&&!name.empty()&&name[0]!='/'&&name.find("..")==std::string::npos
       &&packed<=kMaxInput&&unpacked<=kMaxEntry&&local<bytes.size())
         items.emplace(name,ZipEntry{local,packed,unpacked,method});
     p+=46+nameLen+extra+comment;
   }
   return !items.empty();
 }
 bool get(const std::string& name,std::string& output) const {
   auto it=items.find(name);if(it==items.end())return false;
   const auto& e=it->second;size_t pos=e.offset;
   if(pos+30>bytes.size()||u32(bytes,pos)!=0x04034b50)return false;
   pos+=30+u16(bytes,pos+26)+u16(bytes,pos+28);
   if(pos>bytes.size()||e.compressed>bytes.size()-pos)return false;
   if(e.method==0){output=bytes.substr(pos,e.compressed);return output.size()==e.uncompressed;}
   if(e.method==8)return inflate_raw(bytes.data()+pos,e.compressed,e.uncompressed,output);
   return false;
 }
};
std::string attr(const std::string& tag,const char* name){
 auto p=tag.find(name);const size_t len=std::strlen(name);
 while(p!=std::string::npos){
  if(p>0 && !std::isspace((unsigned char)tag[p-1])&&tag[p-1]!='<'){p=tag.find(name,p+len);continue;}
  size_t q=p+len;while(q<tag.size()&&std::isspace((unsigned char)tag[q]))++q;
  if(q>=tag.size()||tag[q]!='='){p=tag.find(name,p+len);continue;}
  ++q;while(q<tag.size()&&std::isspace((unsigned char)tag[q]))++q;
  if(q>=tag.size()||(tag[q]!='"'&&tag[q]!='\''))return {};
  char quote=tag[q++];size_t end=tag.find(quote,q);
  return end==std::string::npos?std::string():tag.substr(q,end-q);
 }
 return {};
}
// Locate an XML start element by its *complete* local name. Substring
// searches for "<rootfile" also match "<rootfiles>", the EPUB container
// wrapper, causing a missing full-path error in otherwise valid EPUBs.
size_t xml_start(const std::string& xml,const char* local,size_t from=0){
 while((from=xml.find('<',from))!=std::string::npos){
  if(xml.compare(from,4,"<!--")==0){
   const size_t end=xml.find("-->",from+4);
   if(end==std::string::npos)return std::string::npos;
   from=end+3;continue;
  }
  size_t p=from+1;
  if(p>=xml.size())return std::string::npos;
  const auto first=(unsigned char)xml[p];
  if(!std::isalpha(first)&&first!='_'){from=p;continue;}
  const size_t start=p;
  while(p<xml.size()){
   const unsigned char c=(unsigned char)xml[p];
   if(!std::isalnum(c)&&c!='_'&&c!='-'&&c!='.'&&c!=':')break;
   ++p;
  }
  const std::string qualified=xml.substr(start,p-start);
  const auto colon=qualified.find_last_of(':');
  const std::string name=qualified.substr(colon==std::string::npos?0:colon+1);
  if(name==local)return from;
  from=p;
 }
 return std::string::npos;
}
std::string tag_text(const std::string& s,const char* tag){
 std::string needle="<";needle+=tag;
 size_t p=s.find(needle);if(p==std::string::npos)return {};
 p=s.find('>',p);if(p==std::string::npos)return {};
 auto q=s.find('<',p+1);return q==std::string::npos?std::string():s.substr(p+1,q-p-1);
}
std::string resolve(const std::string& base,const std::string& rel){
 if(rel.empty()||rel[0]=='/'||rel.find("://")!=std::string::npos)return {};
 const size_t query=rel.find_first_of("?#");
 std::string path=base.substr(0,base.find_last_of('/')==std::string::npos?0:base.find_last_of('/')+1)
          +rel.substr(0,query);
 std::vector<std::string> parts;size_t begin=0;
 while(begin<path.size()){
  auto end=path.find('/',begin);if(end==std::string::npos)end=path.size();
  std::string v=path.substr(begin,end-begin);
  if(v==".."){if(parts.empty())return {};parts.pop_back();}
  else if(v!="."&&!v.empty())parts.push_back(v);
  begin=end+1;
 }
 std::string final;
 for(auto& x:parts){if(!final.empty())final+='/';final+=x;}
 return final;
}
void codepoint(std::string& out,unsigned n){
 if(n==0||n>0x10ffff)return;
 if(n<128)out.push_back((char)n);
 else if(n<2048){out.push_back((char)(0xc0|(n>>6)));out.push_back((char)(0x80|(n&63)));}
 else if(n<65536){out.push_back((char)(0xe0|(n>>12)));out.push_back((char)(0x80|((n>>6)&63)));out.push_back((char)(0x80|(n&63)));}
 else {out.push_back((char)(0xf0|(n>>18)));out.push_back((char)(0x80|((n>>12)&63)));out.push_back((char)(0x80|((n>>6)&63)));out.push_back((char)(0x80|(n&63)));}
}
std::string decode(const std::string& s){
 std::string out;out.reserve(s.size());
 for(size_t i=0;i<s.size();++i){
  if(s[i]!='&'){out+=s[i];continue;}
  size_t semi=s.find(';',i+1);
  if(semi==std::string::npos||semi-i>12){out+='&';continue;}
  std::string key=s.substr(i+1,semi-i-1);unsigned val=0;
  if(key=="amp")val='&';else if(key=="lt")val='<';else if(key=="gt")val='>';
  else if(key=="quot")val='"';else if(key=="apos")val='\'';
  else if(key=="nbsp")val=' ';else if(key=="mdash")val=0x2014;
  else if(key=="ndash")val=0x2013;else if(key=="rsquo")val=0x2019;
  else if(key=="ldquo")val=0x201c;else if(key=="rdquo")val=0x201d;
  else if(key.size()>1&&key[0]=='#'){val=(unsigned)std::strtoul(key.c_str()+(key[1]=='x'||key[1]=='X'?2:1),nullptr,
     (key[1]=='x'||key[1]=='X')?16:10);}
  if(val){codepoint(out,val);i=semi;}else out+='&';
 }
 return out;
}
std::string strip_markup(const std::string& xml){
 std::string out;out.reserve(xml.size());
 bool skip=false;
 for(size_t i=0;i<xml.size();){
   if(xml[i]=='<'){
    size_t j=xml.find('>',i+1);if(j==std::string::npos)break;
    auto tag=lower(xml.substr(i+1,std::min<size_t>(j-i,100)));
    if(tag.find("script")==0||tag.find("style")==0||tag.find("svg")==0)skip=true;
    if(tag.find("/script")==0||tag.find("/style")==0||tag.find("/svg")==0)skip=false;
    if(!skip && (tag.find("p")==0||tag.find("/p")==0||tag.find("br")==0||
      tag.find("div")==0||tag.find("/div")==0||tag.find("h1")==0||tag.find("/h1")==0||
      tag.find("h2")==0||tag.find("/h2")==0||tag.find("li")==0||tag.find("/li")==0))
      out+='\n';
    i=j+1;continue;
   }
   if(!skip)out+=xml[i];++i;
 }
 return decode(out);
}
void append_lines(Document& d,const std::string& text){
 // The font is proportional. Restrict by conservative character count so
 // every page stays within its 2-column paper view on a 1080p display.
 std::string line,word;
 auto flush=[&](){
   if(line.empty()){if(d.lines.empty()||!d.lines.back().empty())d.lines.emplace_back();return;}
   d.lines.push_back(line);line.clear();
 };
 auto add=[&](){
   if(word.empty())return;
   if(word.size()>64)word.resize(64);
   if(line.size()+word.size()+1>49)flush();
   if(!line.empty())line+=' ';
   line+=word;word.clear();
 };
 for(unsigned char c:text){
   if(c=='\r')continue;
   if(c=='\n'){add();flush();continue;}
   if(c==' '||c=='\t'){add();continue;}
   if(c>=32)word+=(char)c;
   if(d.lines.size()>100000)break;
 }
 add();if(!line.empty())flush();
}

// EPUB asset handling deliberately stays within this book's ZIP archive.
// No external image URLs, HTML scripts, SVG or filesystem paths are executed.
struct ManifestImage {std::string id,path,media,properties;};
std::vector<ManifestImage> image_manifest(const std::string& opf,const std::string& opfPath){
 std::vector<ManifestImage> result;
 size_t p=0;
 while((p=xml_start(opf,"item",p))!=std::string::npos && result.size()<3000){
  const size_t q=opf.find('>',p);if(q==std::string::npos)break;
  const std::string tag=opf.substr(p,q-p+1);p=q+1;
  const std::string href=decode(attr(tag,"href"));
  const std::string location=resolve(opfPath,href);
  if(location.empty())continue;
  result.push_back({attr(tag,"id"),location,lower(attr(tag,"media-type")),attr(tag,"properties")});
 }
 return result;
}
bool supported_picture(const std::string& path){
 return ends(path,".jpg")||ends(path,".jpeg")||ends(path,".png")||ends(path,".gif");
}
std::string picture_from_zip(const Zip& zip,const std::string& path){
 if(!supported_picture(path))return {};
 std::string bytes;
 if(!zip.get(path,bytes)||bytes.empty()||bytes.size()>3*1024*1024)return {};
 return bytes;
}
std::string package_cover(const Zip& zip,const std::string& opf,const std::string& opfPath){
 auto images=image_manifest(opf,opfPath);
 std::string targetId;
 size_t p=0;
 while((p=xml_start(opf,"meta",p))!=std::string::npos){
  const auto q=opf.find('>',p);if(q==std::string::npos)break;
  const auto tag=opf.substr(p,q-p+1);p=q+1;
  if(lower(attr(tag,"name"))=="cover"){targetId=attr(tag,"content");break;}
 }
 // EPUB3 cover-image has priority over EPUB2's legacy meta element.
 for(const auto& im:images){
  if(im.properties.find("cover-image")!=std::string::npos && supported_picture(im.path)){
   auto bytes=picture_from_zip(zip,im.path);if(!bytes.empty())return bytes;
  }
 }
 for(const auto& im:images){
  if(im.id==targetId && !targetId.empty() && supported_picture(im.path)){
   auto bytes=picture_from_zip(zip,im.path);if(!bytes.empty())return bytes;
  }
 }
 // EPUBs without cover metadata sometimes use a cover.xhtml wrapper.
 // Prefer a file with 'cover' in its filename over arbitrary chapter illustrations.
 for(const auto& im:images){
  if(lower(im.path).find("cover")!=std::string::npos && supported_picture(im.path)){
   auto bytes=picture_from_zip(zip,im.path);if(!bytes.empty())return bytes;
  }
 }
 for(const auto& im:images)if(supported_picture(im.path)){
  auto bytes=picture_from_zip(zip,im.path);if(!bytes.empty())return bytes;
 }
 return {};
}
bool load_epub_opf(const Zip& zip,std::string& opfPath,std::string& opf,std::string& error){
 std::string container;
 if(!zip.get("META-INF/container.xml",container)){error="Missing EPUB container.xml";return false;}
 size_t p=0,q=0;
 for(p=xml_start(container,"rootfile");p!=std::string::npos;p=xml_start(container,"rootfile",q+1)){
  q=container.find('>',p);if(q==std::string::npos)break;
  opfPath=resolve("",decode(attr(container.substr(p,q-p+1),"full-path")));
  if(!opfPath.empty())break;
 }
 if(opfPath.empty()){error="Cannot resolve EPUB package";return false;}
 if(!zip.get(opfPath,opf)){error="Cannot read EPUB package: "+opfPath;return false;}
 return true;
}
void append_chapter_with_images(Document& doc,const Zip& zip,const std::string& chapter,
                                const std::string& chapterPath){
 size_t cursor=0,search=0;
 while(search<chapter.size() && doc.images.size()<48 && doc.lines.size()<100000){
  size_t p=chapter.find('<',search);
  if(p==std::string::npos)break;
  size_t at=p+1;
  if(at>=chapter.size() || chapter[at]=='/' || chapter[at]=='!'){search=p+1;continue;}
  size_t nameEnd=at;
  while(nameEnd<chapter.size() && (std::isalnum((unsigned char)chapter[nameEnd])||
       chapter[nameEnd]==':'||chapter[nameEnd]=='-'||chapter[nameEnd]=='_'))++nameEnd;
  const std::string element=lower(chapter.substr(at,nameEnd-at));
  const auto colon=element.find_last_of(':');
  const auto name=element.substr(colon==std::string::npos?0:colon+1);
  if(name!="img" && name!="image"){search=p+1;continue;}
  const size_t end=chapter.find('>',nameEnd);
  if(end==std::string::npos)break;
  const std::string tag=chapter.substr(p,end-p+1);
  auto href=decode(attr(tag,"src"));
  if(href.empty())href=decode(attr(tag,"href"));
  if(href.empty())href=decode(attr(tag,"xlink:href"));
  const auto internal=resolve(chapterPath,href);
  std::string bytes=picture_from_zip(zip,internal);
  if(!bytes.empty()){
   append_lines(doc,strip_markup(chapter.substr(cursor,p-cursor)));
   const size_t row=doc.lines.size()%17;
   if(row>10){for(size_t k=row;k<17;k++)doc.lines.emplace_back();}
   InlineImage im;im.line=doc.lines.size();im.encoded=std::move(bytes);
   im.alt=decode(attr(tag,"alt"));
   if(im.alt.size()>120)im.alt.resize(120);
   doc.images.push_back(std::move(im));
   for(int k=0;k<7;k++)doc.lines.emplace_back();
   cursor=end+1;
  }
  search=end+1;
 }
 if(cursor<chapter.size())append_lines(doc,strip_markup(chapter.substr(cursor)));
}
bool epub(const std::string& path,Document& out){
 Zip zip;if(!zip.open(path)){out.error="EPUB ZIP container is invalid or too large";return false;}
 std::string opfPath,opf;
 if(!load_epub_opf(zip,opfPath,opf,out.error))return false;
 out.coverImage=package_cover(zip,opf,opfPath);
 size_t p=0,q=0;
 out.title=decode(tag_text(opf,"dc:title"));
 out.author=decode(tag_text(opf,"dc:creator"));
 std::map<std::string,std::string> manifest;
 p=0;
 while((p=xml_start(opf,"item",p))!=std::string::npos){
  q=opf.find('>',p);if(q==std::string::npos)break;
  auto tag=opf.substr(p,q-p+1);p=q+1;
  const auto id=attr(tag,"id"),href=attr(tag,"href");
  if(!id.empty()&&!href.empty())manifest[id]=resolve(opfPath,href);
 }
 p=0;size_t total=0;
 while((p=xml_start(opf,"itemref",p))!=std::string::npos && total<1500){
  q=opf.find('>',p);if(q==std::string::npos)break;
  auto tag=opf.substr(p,q-p+1);p=q+1;
  const auto id=attr(tag,"idref");
  auto it=manifest.find(id);if(it==manifest.end()||it->second.empty())continue;
  std::string chapter;if(!zip.get(it->second,chapter))continue;
  if(chapter.size()>kMaxEntry)continue;
  out.chapterStarts.push_back(out.pageCount());
  append_chapter_with_images(out,zip,chapter,it->second);
  out.lines.emplace_back();
  ++total;
 }
 if(out.lines.empty()){out.error="No readable XHTML spine chapters";return false;}
 return true;
}
std::string pdf_string(const std::string& src,size_t& p){
 std::string value;int nest=1; // p points after '('
 while(p<src.size()&&nest){
  char c=src[p++];
  if(c=='\\'){
   if(p>=src.size())break;char e=src[p++];
   if(e=='n')value+='\n';else if(e=='r')value+='\n';else if(e=='t')value+=' ';
   else if(e>='0'&&e<='7'){
    int n=e-'0';for(int k=0;k<2&&p<src.size()&&src[p]>='0'&&src[p]<='7';k++)n=n*8+(src[p++]-'0');
    value+=(char)n;
   }else if(e!='\n'&&e!='\r')value+=e;
  }else if(c=='('){nest++;value+=c;}
  else if(c==')'){nest--;if(nest)value+=c;}
  else value+=c;
  if(value.size()>kMaxEntry)break;
 }
 return value;
}
bool pdf(const std::string& path,Document& out){
 std::string bytes;if(!load(path,bytes)){out.error="Unable to read PDF or exceeds 128 MiB";return false;}
 if(bytes.compare(0,5,"%PDF-")!=0){out.error="Invalid PDF header";return false;}
 out.isPdfTextPreview=true;
 out.title=clean_title(path);
 std::string all;
 size_t pos=0;
 // Decode plain and Flate streams, then extract show-text instructions.
 while((pos=bytes.find("stream",pos))!=std::string::npos&&all.size()<kMaxInput){
  if(pos>0 && std::isalnum((unsigned char)bytes[pos-1])){pos+=6;continue;}
  size_t begin=pos+6;
  if(begin<bytes.size()&&bytes[begin]=='\r')begin++;
  if(begin<bytes.size()&&bytes[begin]=='\n')begin++;
  size_t end=bytes.find("endstream",begin);
  if(end==std::string::npos)break;
  std::string stream=bytes.substr(begin,std::min(end-begin,kMaxEntry));
  const auto dictStart=bytes.rfind("<<",pos);
  bool deflate=false;
  if(dictStart!=std::string::npos&&pos-dictStart<4096){
   auto dict=bytes.substr(dictStart,pos-dictStart);
   deflate=dict.find("/FlateDecode")!=std::string::npos;
  }
  if(deflate){
   z_stream z{};if(inflateInit(&z)==Z_OK){
    std::string decoded;z.next_in=(Bytef*)stream.data();z.avail_in=(uInt)stream.size();
    char b[8192];int rc=Z_OK;
    while(rc==Z_OK&&decoded.size()<kMaxEntry){
      z.next_out=(Bytef*)b;z.avail_out=sizeof(b);rc=inflate(&z,Z_NO_FLUSH);
      decoded.append(b,sizeof(b)-z.avail_out);
    }
    inflateEnd(&z);
    if(rc==Z_STREAM_END)stream=std::move(decoded);
    else stream.clear();
   }
  }
  if(!stream.empty()){
    size_t bt=0;
    while((bt=stream.find("BT",bt))!=std::string::npos){
      auto et=stream.find("ET",bt+2);if(et==std::string::npos)break;
      auto body=stream.substr(bt+2,et-bt-2);
      bool had=false;
      for(size_t k=0;k<body.size();++k){
       if(body[k]=='('){
        ++k;auto value=pdf_string(body,k);
        // Accept ordinary PDF printable text. Custom font encodings may fail.
        for(char& c:value)if((unsigned char)c<32&&c!='\n')c=' ';
        if(value.find_first_not_of(" \n\t")!=std::string::npos){
          all+=value;all+=' ';had=true;
        }
       }
      }
      if(had)all+='\n';bt=et+2;
    }
  }
  pos=end+9;
 }
 if(all.empty()){out.error="This PDF requires a full graphics/font PDF renderer (not available in v0.1.0)";return false;}
 append_lines(out,all);return !out.lines.empty();
}
bool isbook(const std::string& s){return ends(s,".epub")||ends(s,".pdf")||ends(s,".cbr")||ends(s,".cbz");}
constexpr size_t kMaxComic=512*1024*1024;
std::vector<ScanSource> lastReport;
// Scan ordinary folders, nested author folders, and USB mounts. Keep all work
// bounded: max 4 directory levels, max 12000 examined entries per source.
void collect(const std::string& path,std::vector<BookEntry>& out,
             std::set<std::string>& seen,ScanSource& status,
             size_t& remaining,int depth=0) {
 DIR* d=opendir(path.c_str());
 if(!d) {
   if(depth==0)status.error=std::strerror(errno);
   return;
 }
 if(depth==0)status.opened=true;
 ++status.folders;
 struct dirent* entry;
 while(remaining>0 && (entry=readdir(d))!=nullptr) {
  --remaining;
  std::string name=entry->d_name;
  if(name=="."||name==".."||name.empty()||name.size()>240)continue;
  std::string full=path+"/"+name;
  if(full.size()>1000)continue;
  struct stat st{};
  const bool stat_ok=stat(full.c_str(),&st)==0;
  // Some homebrew/USB layers do not provide a usable st_mode. For
  // book candidates, fall back to fopen/fseek so readable files appear.
  if(isbook(name)){
    ++status.candidates;
    if(stat_ok && S_ISDIR(st.st_mode))continue;
    bool valid=false;
    if(stat_ok && S_ISREG(st.st_mode) && st.st_size>=0 &&
       st.st_size<=static_cast<off_t>(ends(name,".cbr")||ends(name,".cbz")?kMaxComic:kMaxInput))valid=true;
    if(!valid && (!stat_ok || !S_ISREG(st.st_mode))){
      FILE* fp=fopen(full.c_str(),"rb");
      if(fp){
        if(fseek(fp,0,SEEK_END)==0){
          long sz=ftell(fp);
          valid=sz>=0 && (size_t)sz<=(ends(name,".cbr")||ends(name,".cbz")?kMaxComic:kMaxInput);
        }
        fclose(fp);
      }
    }
    if(!valid)continue;
    if(!seen.insert(full).second)continue;
    BookEntry b;b.path=full;b.title=clean_title(name);
    b.format=ends(name,".pdf")?"PDF":ends(name,".cbr")?"CBR":ends(name,".cbz")?"CBZ":"EPUB";
    b.collection=(b.format=="CBR"||b.format=="CBZ"||
                  lower(name).find("comic")!=std::string::npos)?"Comics":"Fiction";
    b.author="Unknown Author";
    out.push_back(std::move(b));
    ++status.books;
    continue;
  }
  if(depth>=3)continue;
  bool is_directory=stat_ok&&S_ISDIR(st.st_mode);
  if(!is_directory && !stat_ok){
    DIR* nested=opendir(full.c_str());
    if(nested){closedir(nested);is_directory=true;}
  }
  if(is_directory)collect(full,out,seen,status,remaining,depth+1);
 }
 if(remaining==0 && status.error.empty())status.error="Scan limit reached";
 closedir(d);
}
std::string escape_state(const std::string& s){
 std::string out=s;for(char& c:out)if(c=='\t'||c=='\n'||c=='\r')c=' ';return out;
}
std::string dir(){
 constexpr const char* dirs[]={"/download0/ebook-reader","/data/ebooks/.state",
  "/data/homebrew/PPSA99741/userdata"};
 for(auto path:dirs){
   auto slash=std::string(path).find_last_of('/');
   if(slash!=std::string::npos){auto base=std::string(path).substr(0,slash);mkdir(base.c_str(),0755);}
   if(mkdir(path,0755)==0||errno==EEXIST){
    const auto probe=std::string(path)+"/.write-test";
    FILE* f=std::fopen(probe.c_str(),"wb");
    if(f){std::fclose(f);std::remove(probe.c_str());return path;}
   }
 }
 return {};
}
} // namespace
std::string clean_title(std::string name){
 auto slash=name.find_last_of("/\\");if(slash!=std::string::npos)name=name.substr(slash+1);
 if(ends(name,".epub"))name.resize(name.size()-5);
 if(ends(name,".pdf"))name.resize(name.size()-4);
 if(ends(name,".cbr")||ends(name,".cbz"))name.resize(name.size()-4);
 for(char& c:name)if(c=='_'||c=='-')c=' ';
 return name;
}
bool read_epub_cover(const std::string& path,std::string& encoded){
 encoded.clear();
 Zip zip;if(!zip.open(path))return false;
 std::string opf,opfPath,error;
 if(!load_epub_opf(zip,opfPath,opf,error))return false;
 encoded=package_cover(zip,opf,opfPath);
 return !encoded.empty();
}
bool read_document(const std::string& path,Document& out){
 out=Document{};out.format=ends(path,".pdf")?"PDF":"EPUB";
 bool ok=out.format=="PDF"?pdf(path,out):epub(path,out);
 if(out.title.empty())out.title=clean_title(path);
 return ok;
}
const std::vector<ScanSource>& last_scan_report(){return lastReport;}
bool scan_books(const std::vector<std::string>& dirs,std::vector<BookEntry>& books){
 std::set<std::string> seen;books.clear();lastReport.clear();
 for(const auto& root:dirs){
   ScanSource status;status.path=root;size_t remaining=12000;
   collect(root,books,seen,status,remaining);
   lastReport.push_back(std::move(status));
 }
 std::sort(books.begin(),books.end(),[](const BookEntry& a,const BookEntry& b){
   return lower(a.title)<lower(b.title);
 });
 load_state(books);return true;
}
bool scan_imports(std::vector<BookEntry>& found){
 std::set<std::string> seen;found.clear();lastReport.clear();
 for(const char* root:{
     "/app0/assets/Books","/download0/Books","/download0/ebooks",
     "/mnt/usb0/Books","/mnt/usb0/books","/mnt/usb0/BOOKS",
     "/mnt/usb0","/mnt/usb1/Books","/mnt/usb1","/mnt/usb2"}) {
    ScanSource status;status.path=root;size_t remaining=12000;
    collect(root,found,seen,status,remaining);
    lastReport.push_back(std::move(status));
 }
 std::sort(found.begin(),found.end(),[](const BookEntry& a,const BookEntry& b){
    return lower(a.title)<lower(b.title);
 });
 return true;
}
std::string state_directory(){return dir();}
void load_state(std::vector<BookEntry>& books){
 const auto root=dir();if(root.empty())return;
 std::string content;if(!load(root+"/library.tsv",content,1024*1024))return;
 size_t begin=0;while(begin<content.size()){
  size_t end=content.find('\n',begin);if(end==std::string::npos)end=content.size();
  std::string row=content.substr(begin,end-begin);
  std::vector<std::string> fields;size_t pos=0;
  while(pos<=row.size()){
   size_t tab=row.find('\t',pos);if(tab==std::string::npos)tab=row.size();
   fields.push_back(row.substr(pos,tab-pos));pos=tab+1;
  }
  if(fields.size()>=4)for(auto& b:books)if(b.path==fields[0]){
    b.progress=(size_t)std::strtoul(fields[1].c_str(),nullptr,10);
    b.favorite=fields[2]=="1";
    b.collection=fields[3];
    break;
  }
  begin=end+1;
 }
}
bool save_state(const std::vector<BookEntry>& books){
 const auto root=dir();if(root.empty())return false;
 const auto temporary=root+"/library.tsv.tmp",target=root+"/library.tsv";
 FILE* f=std::fopen(temporary.c_str(),"wb");if(!f)return false;
 bool ok=true;
 for(auto& b:books){
  std::string row=escape_state(b.path)+"\t"+std::to_string(b.progress)+"\t"+
    (b.favorite?"1":"0")+"\t"+escape_state(b.collection)+"\n";
  if(std::fwrite(row.data(),1,row.size(),f)!=row.size()){ok=false;break;}
 }
 if(std::fflush(f)!=0)ok=false;
 if(std::fclose(f)!=0)ok=false;
 if(!ok){std::remove(temporary.c_str());return false;}
 if(std::rename(temporary.c_str(),target.c_str())!=0){std::remove(temporary.c_str());return false;}
 return true;
}
bool import_to_library(const std::string& source,std::string& result){
 if(!isbook(source)||source.find("..")!=std::string::npos){result="Invalid filename";return false;}
 struct stat st{};if(stat(source.c_str(),&st)!=0||!S_ISREG(st.st_mode)||st.st_size>int64_t(kMaxComic)){
   result="Source file unavailable or too large (128 MiB limit)";return false;
 }
 // The ordinary app sandbox can write /download0 but may be denied
 // /data/ebooks. Do not assume console-wide filesystem elevation.
 std::string folder;
 for(const char* candidate:{"/download0/Books","/data/ebooks"}){
   if(mkdir(candidate,0755)!=0&&errno!=EEXIST)continue;
   std::string test=std::string(candidate)+"/.ebook-write-probe";
   FILE* check=std::fopen(test.c_str(),"wb");
   if(check){std::fclose(check);std::remove(test.c_str());folder=candidate;break;}
 }
 if(folder.empty()){result="No writable library: check /download0 access";return false;}
 auto slash=source.find_last_of('/');const auto name=source.substr(slash+1);
 std::string dest=folder+"/"+name;
 if(access(dest.c_str(),F_OK)==0){result="Already imported: "+name;return true;}
 FILE* in=std::fopen(source.c_str(),"rb");if(!in){result="Unable to read USB file";return false;}
 const auto temp=dest+".partial";
 FILE* out=std::fopen(temp.c_str(),"wb");if(!out){std::fclose(in);result="Library is not writable";return false;}
 char buf[64*1024];size_t bytes;bool ok=true;
 while((bytes=std::fread(buf,1,sizeof(buf),in))!=0){
  if(std::fwrite(buf,1,bytes,out)!=bytes){ok=false;break;}
 }
 if(std::ferror(in))ok=false;
 if(std::fflush(out)!=0)ok=false;
 if(std::fclose(out)!=0)ok=false;
 std::fclose(in);
 if(!ok||std::rename(temp.c_str(),dest.c_str())!=0){
  std::remove(temp.c_str());result="Import failed or disk full";return false;
 }
 result="Imported: "+name;return true;
}
bool erase_imported(const BookEntry& b){
 if(b.path.rfind("/data/ebooks/",0)!=0 || b.path.find("..")!=std::string::npos)return false;
 return std::remove(b.path.c_str())==0;
}
} // namespace ebook
