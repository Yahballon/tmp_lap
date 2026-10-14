// SPDX-License-Identifier: GPL-3.0-or-later
#include "transfer.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "ca_bundle.inc"
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <climits>
#include <algorithm>
#include <string_view>
namespace pstube {
namespace {
struct Url {std::string host,path;};
bool parse_url(const std::string& url,Url& out){
 if(url.rfind("https://",0)!=0||url.size()>12000)return false;
 auto start=(size_t)8,end=url.find('/',start);
 if(end==std::string::npos)end=url.size();
 out.host=url.substr(start,end-start);
 out.path=end==url.size()?"/":url.substr(end);
 if(out.host.empty()||out.host.size()>180||out.host.front()=='.'||out.host.back()=='.')return false;
 for(unsigned char c:out.host)if(!std::isalnum(c)&&c!='-'&&c!='.')return false;
 if(out.host.find('.')==std::string::npos||out.host.find("..")!=std::string::npos)return false;
 return out.path.find('\r')==std::string::npos&&out.path.find('\n')==std::string::npos&&out.path.find('#')==std::string::npos;
}
bool allowed_host(const std::string& host){
 static const char* providers[]={"googlevideo.com","ytimg.com","yewtu.be","inv.nadeko.net","invidious.nerdvpn.de",
   "yt.chocolatemoo53.com","invidious.tiekoetter.com","inv.tux.pizza","invidious.private.coffee"};
 for(const char* p:providers){std::string d(p);if(host==d||(host.size()>d.size()&&host.compare(host.size()-d.size(),d.size(),d)==0&&host[host.size()-d.size()-1]=='.'))return true;}
 return false;
}
std::string trim(std::string v){while(!v.empty()&&std::isspace((unsigned char)v.back()))v.pop_back();size_t i=0;while(i<v.size()&&std::isspace((unsigned char)v[i]))i++;return v.substr(i);}
std::string header(const std::string& s,const char* key) {
 std::string lower=s,needle=key;
 std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char x){return (char)std::tolower(x);});
 for(size_t start=0;start<lower.size();){size_t end=lower.find("\r\n",start);if(end==std::string::npos)end=lower.size();
  if(lower.compare(start,needle.size(),needle)==0)return trim(s.substr(start+needle.size(),end-start-needle.size()));
  start=end+2;
 }return {};
}
bool once(const Url& url,const std::string& target,std::atomic<int>& progress,std::string& err,std::string& redirect){
 SSL_CTX* ctx=SSL_CTX_new(TLS_client_method());if(!ctx){err="Unable to initialize TLS";return false;}
 SSL_CTX_set_min_proto_version(ctx,TLS1_2_VERSION);SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,nullptr);
 BIO* bio=BIO_new_mem_buf(kCaBundle,-1);int roots=0;
 if(bio){auto* store=SSL_CTX_get_cert_store(ctx);while(X509* cert=PEM_read_bio_X509(bio,nullptr,nullptr,nullptr)){if(X509_STORE_add_cert(store,cert)==1)roots++;X509_free(cert);}BIO_free(bio);ERR_clear_error();}
 if(!roots){SSL_CTX_free(ctx);err="No CA trust anchors";return false;}
 addrinfo hints{};hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;
 addrinfo* addresses=nullptr;if(getaddrinfo(url.host.c_str(),"443",&hints,&addresses)!=0){SSL_CTX_free(ctx);err="Media host DNS failure";return false;}
 int fd=-1;
 for(addrinfo* a=addresses;a;a=a->ai_next){fd=socket(a->ai_family,a->ai_socktype,a->ai_protocol);if(fd<0)continue;
  timeval tv{15,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
  if(connect(fd,a->ai_addr,a->ai_addrlen)==0)break;close(fd);fd=-1;}
 freeaddrinfo(addresses);
 if(fd<0){err="Media host unreachable";SSL_CTX_free(ctx);return false;}
 SSL* ssl=SSL_new(ctx);if(!ssl){close(fd);SSL_CTX_free(ctx);err="TLS connection unavailable";return false;}
 SSL_set_fd(ssl,fd);SSL_set_tlsext_host_name(ssl,url.host.c_str());SSL_set1_host(ssl,url.host.c_str());
 bool success=false;FILE* file=nullptr;
 do {
  if(SSL_connect(ssl)!=1||SSL_get_verify_result(ssl)!=X509_V_OK){err="Media TLS certificate rejected";break;}
  std::string req="GET "+url.path+" HTTP/1.1\r\nHost: "+url.host+"\r\nUser-Agent: PSTUBE/0.2\r\nAccept: */*\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n";
  bool send_ok=true;
  for(size_t off=0;off<req.size();){int n=SSL_write(ssl,req.data()+off,(int)(req.size()-off));if(n<=0){send_ok=false;break;}off+=(size_t)n;}
  if(!send_ok){err="Media request failed";break;}
  std::string response;char buf[32768];int n=0;
  while(response.find("\r\n\r\n")==std::string::npos&&response.size()<32768){
   n=SSL_read(ssl,buf,sizeof(buf));if(n<=0)break;response.append(buf,(size_t)n);
  }
  size_t delim=response.find("\r\n\r\n");
  if(delim==std::string::npos||delim>16384){err="Invalid media HTTP headers";break;}
  std::string heads=response.substr(0,delim);
  int status=0;if(std::sscanf(heads.c_str(),"HTTP/%*s %d",&status)!=1){err="Invalid media status";break;}
  if(status==301||status==302||status==303||status==307||status==308){
   redirect=header(heads,"location:");if(redirect.rfind("https://",0)!=0){err="Unsafe redirect";redirect.clear();}
   break;
  }
  if(status!=200){err="Media server returned HTTP "+std::to_string(status);break;}
  if(!header(heads,"transfer-encoding:").empty()){err="Chunked media response not supported";break;}
  const auto content_length=header(heads,"content-length:");
  unsigned long long expected=0;
  if(!content_length.empty()){
   char* e=nullptr;expected=std::strtoull(content_length.c_str(),&e,10);
   if(e==content_length.c_str()||*e){err="Invalid content length";break;}
  }
  constexpr unsigned long long max_size=2ULL*1024*1024*1024;
  if(expected>max_size){err="Media file exceeds 2 GiB limit";break;}
  file=std::fopen((target+".part").c_str(),"wb");
  if(!file){err="No writable download directory";break;}
  size_t start=delim+4;unsigned long long size=0;
  auto append=[&](const char* bytes,size_t count)->bool{
   if(count>max_size-size){err="Media exceeds 2 GiB limit";return false;}
   if(count&&std::fwrite(bytes,1,count,file)!=count){err="Storage write failed";return false;}
   size+=count;
   if(expected)progress.store((int)std::min(99ULL,size*100/expected));
   return true;
  };
  if(!append(response.data()+start,response.size()-start))break;
  bool read_failed=false;
  for(;;){
   n=SSL_read(ssl,buf,sizeof(buf));
   if(n<=0){int e=SSL_get_error(ssl,n);if(e!=SSL_ERROR_ZERO_RETURN&&e!=SSL_ERROR_SYSCALL){read_failed=true;}break;}
   if(!append(buf,(size_t)n)){read_failed=true;break;}
  }
  if(read_failed){if(err.empty())err="Media transfer interrupted";break;}
  if(expected&&size!=expected){err="Truncated media transfer";break;}
  if(size<1024){err="Downloaded media is empty";break;}
  if(std::fflush(file)!=0||::fsync(fileno(file))!=0){err="Failed to flush media";break;}
  if(std::fclose(file)!=0){file=nullptr;err="Failed to close media file";break;}file=nullptr;
  if(::rename((target+".part").c_str(),target.c_str())!=0){err="Unable to finalize download";break;}
  progress.store(100);success=true;
 }while(false);
 if(file)std::fclose(file);
 SSL_shutdown(ssl);SSL_free(ssl);close(fd);SSL_CTX_free(ctx);
 if(!success&&redirect.empty())::unlink((target+".part").c_str());
 return success;
}
}
bool fetch_mp4(const std::string& input,const std::string& output,std::atomic<int>& progress,std::string& error){
 std::string url=input;
 for(int i=0;i<4;i++){
  Url target;
  if(!parse_url(url,target)||!allowed_host(target.host)){error="Media URL host is not trusted";return false;}
  std::string redirect;
  if(once(target,output,progress,error,redirect))return true;
  if(redirect.empty())return false;
  url=redirect;
 }
 error="Too many media redirects";return false;
}
}
