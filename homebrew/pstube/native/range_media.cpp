// SPDX-License-Identifier: GPL-3.0-or-later
#include "range_media.hpp"
#include "ca_bundle.inc"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <climits>
#include <cstdint>
#include <string>
namespace pstube {
namespace {
constexpr unsigned long long MAX_FILE=2ULL*1024*1024*1024;
constexpr unsigned long long CHUNK=256*1024; // finite Range, never "bytes=0-"
struct Url {std::string host,path;};
bool parse(const std::string& input,Url& out){
 if(input.rfind("https://",0)!=0||input.size()>16000)return false;
 size_t pos=input.find('/',8);
 if(pos==std::string::npos)return false;
 out.host=input.substr(8,pos-8);out.path=input.substr(pos);
 if(out.host.size()<17||out.host.size()>190||out.path.find_first_of("\r\n#")!=std::string::npos)return false;
 for(unsigned char c:out.host)if(!std::isalnum(c)&&c!='-'&&c!='.')return false;
 static const std::string suffix=".googlevideo.com";
 return out.host=="googlevideo.com"||
   (out.host.size()>suffix.size()&&
    out.host.compare(out.host.size()-suffix.size(),suffix.size(),suffix)==0);
}
std::string header(const std::string& heads,const char* name){
 const std::string needle(name);
 for(size_t i=0;i<heads.size();){
  size_t end=heads.find("\r\n",i);
  if(end==std::string::npos)end=heads.size();
  if(end-i>=needle.size()){
   bool found=true;
   for(size_t j=0;j<needle.size();j++)
    if(std::tolower((unsigned char)heads[i+j])!=std::tolower((unsigned char)needle[j])){found=false;break;}
   if(found){
    size_t from=i+needle.size();
    while(from<end && (heads[from]==' '||heads[from]=='\t'))from++;
    return heads.substr(from,end-from);
   }
  }
  if(end==heads.size())break;
  i=end+2;
 }
 return {};
}
struct Response {
 unsigned long long offset=0,last=0,total=0;
};
bool retrieve(const Url& url,unsigned long long offset,
 unsigned long long last,FILE* file,unsigned long long& total,
 std::atomic<bool>& cancel,std::string& error){
 SSL_CTX* ctx=SSL_CTX_new(TLS_client_method());
 if(!ctx){error="Cannot initialize HD TLS";return false;}
 SSL_CTX_set_min_proto_version(ctx,TLS1_2_VERSION);
 SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,nullptr);
 BIO* ca=BIO_new_mem_buf(kCaBundle,-1);int loaded=0;
 if(ca){
  X509_STORE* store=SSL_CTX_get_cert_store(ctx);
  while(X509* cert=PEM_read_bio_X509(ca,nullptr,nullptr,nullptr)){
   if(X509_STORE_add_cert(store,cert)==1)loaded++;
   X509_free(cert);
  }
  BIO_free(ca);ERR_clear_error();
 }
 if(!loaded){SSL_CTX_free(ctx);error="HD certificate roots unavailable";return false;}
 addrinfo hints{};hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;
 addrinfo* nodes=nullptr;
 if(getaddrinfo(url.host.c_str(),"443",&hints,&nodes)!=0){
  SSL_CTX_free(ctx);error="HD CDN DNS failure";return false;
 }
 int fd=-1;
 for(addrinfo* it=nodes;it&&!cancel.load();it=it->ai_next){
  fd=socket(it->ai_family,it->ai_socktype,it->ai_protocol);
  if(fd<0)continue;
  timeval timeout{12,0};
  setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
  setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
  if(connect(fd,it->ai_addr,it->ai_addrlen)==0)break;
  close(fd);fd=-1;
 }
 freeaddrinfo(nodes);
 if(fd<0){SSL_CTX_free(ctx);error=cancel?"Cancelled":"HD CDN connection failed";return false;}
 SSL* ssl=SSL_new(ctx);
 if(!ssl){close(fd);SSL_CTX_free(ctx);error="HD TLS context allocation failed";return false;}
 SSL_set_fd(ssl,fd);
 SSL_set_tlsext_host_name(ssl,url.host.c_str());
 SSL_set1_host(ssl,url.host.c_str());
 bool ok=false;
 do {
  if(cancel.load()){error="Cancelled";break;}
  if(SSL_connect(ssl)!=1||SSL_get_verify_result(ssl)!=X509_V_OK){
   error="HD TLS peer verification failed";break;
  }
  const std::string request=
   "GET "+url.path+" HTTP/1.1\r\nHost: "+url.host+
   "\r\nUser-Agent: com.google.android.apps.youtube.vr.oculus/1.65.10 (Linux; U; Android 12L) gzip"+
   "\r\nAccept: */*\r\nAccept-Encoding: identity\r\nRange: bytes="+
    std::to_string(offset)+"-"+std::to_string(last)+
   "\r\nConnection: close\r\n\r\n";
  size_t written=0;
  while(written<request.size()&&!cancel.load()){
   int n=SSL_write(ssl,request.data()+written,(int)(request.size()-written));
   if(n<=0){error="HD range request failed";break;}written+=(size_t)n;
  }
  if(written!=request.size()){if(error.empty())error="Cancelled";break;}
  std::string response;char chunk[32768]{};
  while(response.find("\r\n\r\n")==std::string::npos&&response.size()<32768&&!cancel.load()){
   int n=SSL_read(ssl,chunk,sizeof(chunk));
   if(n<=0)break;
   response.append(chunk,(size_t)n);
  }
  auto boundary=response.find("\r\n\r\n");
  if(boundary==std::string::npos||boundary>16384){error="Invalid HD range response";break;}
  const auto headers=response.substr(0,boundary);
  int status=0;
  if(std::sscanf(headers.c_str(),"HTTP/%*s %d",&status)!=1){error="Invalid HD HTTP status";break;}
  if(status!=206){error="HD CDN HTTP "+std::to_string(status)+" (finite ranges required)";break;}
  unsigned long long first=0,end=0,size=0;
  const auto ranges=header(headers,"content-range:");
  if(std::sscanf(ranges.c_str(),"bytes %llu-%llu/%llu",&first,&end,&size)!=3
    ||first!=offset||end<first||end>last||size==0||size>MAX_FILE){
   error="Invalid HD Content-Range";break;
  }
  if(total && total!=size){error="HD file size changed during transfer";break;}
  total=size;
  const unsigned long long expected=end-first+1;
  const auto length=header(headers,"content-length:");
  if(!length.empty()){
   char* tail=nullptr;const auto n=std::strtoull(length.c_str(),&tail,10);
   if(!tail||*tail!='\0'||n!=expected){error="Mismatched HD content length";break;}
  }
  unsigned long long received=0;
  auto append=[&](const char* bytes,size_t n)->bool{
   if(n>expected-received){error="Unexpected extra HD data";return false;}
   if(n&&std::fwrite(bytes,1,n,file)!=n){error="HD storage write failed";return false;}
   received+=n;return true;
  };
  const size_t data=boundary+4;
  if(!append(response.data()+data,response.size()-data))break;
  while(received<expected&&!cancel.load()){
   const int n=SSL_read(ssl,chunk,(int)std::min<unsigned long long>(sizeof(chunk),expected-received));
   if(n<=0){error="HD range ended early";break;}
   if(!append(chunk,(size_t)n))break;
  }
  if(cancel.load()){error="Cancelled";break;}
  if(received!=expected){if(error.empty())error="HD range truncated";break;}
  ok=true;
 }while(false);
 SSL_shutdown(ssl);
 SSL_free(ssl);close(fd);SSL_CTX_free(ctx);
 return ok;
}
}
bool fetch_bounded_range_media(const std::string& input,const std::string& target,
 std::atomic<int>& percent,std::atomic<bool>& cancel,std::string& error){
 Url url;error.clear();percent.store(0);
 if(!parse(input,url)){error="Untrusted HD media host";return false;}
 if(target.empty()||target.size()>1024){error="Invalid HD cache path";return false;}
 const std::string tmp=target+".part";
 FILE* file=std::fopen(tmp.c_str(),"wb");
 if(!file){error="HD cache not writable";return false;}
 bool ok=true;
 unsigned long long offset=0,total=0;
 while(!cancel.load()){
  if(total && offset>=total)break;
  const unsigned long long end=offset+CHUNK-1;
  if(!retrieve(url,offset,total?std::min(end,total-1):end,file,total,cancel,error)){
   ok=false;break;
  }
  if(total==0||total>MAX_FILE){ok=false;error="Invalid HD content length";break;}
  offset=std::min(offset+CHUNK,total);
  percent.store((int)std::min(99ULL,offset*100/total));
 }
 if(cancel.load()){ok=false;error="Cancelled";}
 if(ok&&offset!=total){ok=false;error="HD file incomplete";}
 if(ok&&(std::fflush(file)!=0||fsync(fileno(file))!=0)){ok=false;error="HD file sync failed";}
 if(std::fclose(file)!=0){ok=false;error="Cannot close HD cache";}
 if(ok){
  if(std::rename(tmp.c_str(),target.c_str())!=0){ok=false;error="Cannot finalize HD cache";}
 }else unlink(tmp.c_str());
 if(!ok)unlink(tmp.c_str());
 if(ok)percent.store(100);
 return ok;
}
}
