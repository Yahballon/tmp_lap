// SPDX-License-Identifier: GPL-3.0-or-later
#include "pstube.hpp"
#include "transfer.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "ca_bundle.inc"
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string_view>
namespace pstube {
namespace {
constexpr size_t kMaxJson=5*1024*1024;
const char* kSave="/download0/pstube.txt";
const char* kFallback="/data/pstube/state.txt";
bool host_ok(const std::string& h) {
 if(h.empty()||h.size()>128||h.front()=='.'||h.back()=='.')return false;
 for(unsigned char c:h)if(!std::isalnum(c)&&c!='.'&&c!='-')return false;
 return h.find('.')!=std::string::npos&&h.find("..")==std::string::npos;
}
bool https(const std::string& host,const std::string& path,std::string& payload,std::string& error,
           const std::string& postBody="",int redirects=0) {
 if(!host_ok(host)||path.empty()||path.front()!='/'){error="Invalid HTTPS target";return false;}
 SSL_CTX* ctx=SSL_CTX_new(TLS_client_method());
 if(!ctx){error="TLS initialization failed";return false;}
 SSL_CTX_set_min_proto_version(ctx,TLS1_2_VERSION);
 SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,nullptr);
 BIO* bio=BIO_new_mem_buf(kCaBundle,-1);
 int count=0;
 if(bio){auto* store=SSL_CTX_get_cert_store(ctx);while(X509* cert=PEM_read_bio_X509(bio,nullptr,nullptr,nullptr)){if(X509_STORE_add_cert(store,cert)==1)++count;X509_free(cert);}BIO_free(bio);ERR_clear_error();}
 if(!count){error="Trusted CA bundle unavailable";SSL_CTX_free(ctx);return false;}
 addrinfo hints{};hints.ai_socktype=SOCK_STREAM;hints.ai_family=AF_UNSPEC;
 addrinfo* addresses=nullptr;
 if(getaddrinfo(host.c_str(),"443",&hints,&addresses)!=0){error="DNS lookup failed";SSL_CTX_free(ctx);return false;}
 int fd=-1;
 for(auto* a=addresses;a;a=a->ai_next){
  fd=socket(a->ai_family,a->ai_socktype,a->ai_protocol);if(fd<0)continue;
  timeval tv{5,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
  if(connect(fd,a->ai_addr,a->ai_addrlen)==0)break;
  close(fd);fd=-1;
 }
 freeaddrinfo(addresses);
 if(fd<0){error="Connection unavailable";SSL_CTX_free(ctx);return false;}
 SSL* ssl=SSL_new(ctx);
 if(!ssl){close(fd);SSL_CTX_free(ctx);error="TLS allocation failed";return false;}
 SSL_set_fd(ssl,fd);SSL_set_tlsext_host_name(ssl,host.c_str());SSL_set1_host(ssl,host.c_str());
 if(SSL_connect(ssl)!=1||SSL_get_verify_result(ssl)!=X509_V_OK){error="TLS verification failed";SSL_free(ssl);close(fd);SSL_CTX_free(ctx);return false;}
 std::string request=(postBody.empty()?"GET ":"POST ")+path+
 " HTTP/1.1\r\nHost: "+host+
 "\r\nUser-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36\r\nAccept: text/html,application/json;q=0.9,*/*;q=0.8\r\nAccept-Encoding: identity\r\nConnection: close\r\n";
 if(!postBody.empty()){
  request+="Content-Type: application/json\r\nOrigin: https://www.youtube.com\r\nContent-Length: "+std::to_string(postBody.size())+"\r\n";
  if(postBody.find("\"clientName\":\"ANDROID\"")!=std::string::npos){
   request+="X-YouTube-Client-Name: 3\r\nX-YouTube-Client-Version: 21.26.364\r\n";
  }
 }
 request+="\r\n"+postBody;
 bool ok=true;
 for(size_t off=0;off<request.size();){
  int n=SSL_write(ssl,request.data()+off,(int)(request.size()-off));if(n<=0){ok=false;error="HTTP send failed";break;}off+=(size_t)n;
 }
 std::string response;
 if(ok){char buf[8192];for(;;){int n=SSL_read(ssl,buf,sizeof(buf));if(n<=0)break;if(response.size()+(size_t)n>kMaxJson+8192){ok=false;error="Response too large";break;}response.append(buf,(size_t)n);}}
 SSL_shutdown(ssl);SSL_free(ssl);close(fd);SSL_CTX_free(ctx);
 if(!ok)return false;
 size_t sep=response.find("\r\n\r\n");if(sep==std::string::npos){error="Invalid HTTP response";return false;}
 std::string headers=response.substr(0,sep),lower=headers;
 int code=0;if(std::sscanf(headers.c_str(),"HTTP/%*s %d",&code)!=1){error="Invalid HTTP status";return false;}
 if(code==301||code==302||code==303||code==307||code==308){
  if(redirects>=3){error="Too many HTTPS redirects";return false;}
  const auto start=headers.find("Location: ");
  const auto lowerStart=headers.find("location: ");
  const auto at=start!=std::string::npos?start:lowerStart;
  if(at==std::string::npos){error="HTTP redirect missing Location";return false;}
  const size_t valueStart=at+(start!=std::string::npos?10:10);
  const auto end=headers.find("\r\n",valueStart);
  const std::string location=headers.substr(valueStart,end==std::string::npos?std::string::npos:end-valueStart);
  // Restrict redirects to the original HTTPS YouTube host to avoid credential leaks.
  std::string safePath=location;
  if(safePath.rfind("https://www.youtube.com/",0)==0)safePath=safePath.substr(23);
  else if(safePath.rfind("/",0)!=0){error="Unsupported redirect destination";return false;}
  if(host!="www.youtube.com"||safePath.size()>2048||safePath.find("\r")!=std::string::npos||safePath.find("\n")!=std::string::npos){
   error="Unexpected YouTube redirect";return false;
  }
  return https(host,safePath,payload,error,{},redirects+1);
 }
 if(code!=200){error="Service HTTP "+std::to_string(code);return false;}
 std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return (char)std::tolower(c);});
 std::string_view body(response.data()+sep+4,response.size()-sep-4);
 if(lower.find("transfer-encoding: chunked")==std::string::npos)payload.assign(body);
 else {
  size_t p=0;
  while(p<body.size()){
   size_t line=body.find("\r\n",p);if(line==std::string_view::npos||line-p>40){error="Invalid chunked response";return false;}
   std::string part(body.substr(p,line-p));const size_t semicolon=part.find(';');if(semicolon!=std::string::npos)part.resize(semicolon);
   char* end=nullptr;unsigned long n=strtoul(part.c_str(),&end,16);
   if(end==part.c_str()||*end||n>kMaxJson||payload.size()+n>kMaxJson){error="Invalid chunk size";return false;}
   p=line+2;if(n==0)break;
   if(n>body.size()-p){error="Truncated chunk";return false;}
   payload.append(body.data()+p,n);p+=n;
   if(p+2>body.size()||body.substr(p,2)!="\r\n"){error="Invalid chunk separator";return false;}p+=2;
  }
 }
 return !payload.empty();
}
void skip(std::string_view s,size_t& p){while(p<s.size()&&(s[p]==' '||s[p]=='\t'||s[p]=='\r'||s[p]=='\n'))++p;}
size_t value_end(std::string_view s,size_t p){
 if(p>=s.size())return p;
 char c=s[p];
 if(c=='"'){
  for(size_t i=p+1;i<s.size();++i){if(s[i]=='\\'){++i;continue;}if(s[i]=='"')return i+1;}return s.size();
 }
 if(c=='{'||c=='['){char close=c=='{'?'}':']';char open=c;int depth=0;bool str=false;
  for(size_t i=p;i<s.size();++i){if(str){if(s[i]=='\\'){++i;continue;}if(s[i]=='"')str=false;}else {if(s[i]=='"')str=true;else if(s[i]==open)depth++;else if(s[i]==close&&--depth==0)return i+1;}}return s.size();
 }
 size_t e=p;while(e<s.size()&&s[e]!=','&&s[e]!='}'&&s[e]!=']'&&!std::isspace((unsigned char)s[e]))++e;return e;
}
std::string_view field(std::string_view obj,std::string_view key){
 size_t p=0;skip(obj,p);if(p==obj.size()||obj[p++]!='{')return {};
 while(p<obj.size()){skip(obj,p);if(p>=obj.size()||obj[p]=='}'||obj[p]!='"')break;
  size_t end=value_end(obj,p);std::string_view k=obj.substr(p+1,end-p-2);p=end;skip(obj,p);if(p>=obj.size()||obj[p++]!=':')break;skip(obj,p);
  size_t e=value_end(obj,p);if(k==key)return obj.substr(p,e-p);
  p=e;skip(obj,p);if(p>=obj.size()||obj[p++]!=',')break;
 }return {};
}
std::string decode(std::string_view s,size_t limit=512){
 if(s.size()<2||s.front()!='"')return {};
 std::string result;result.reserve(std::min(limit,s.size()));
 auto hx=[](char c)->int{
  if(c>='0'&&c<='9')return c-'0';
  if(c>='a'&&c<='f')return c-'a'+10;
  if(c>='A'&&c<='F')return c-'A'+10;
  return -1;
 };
 for(size_t p=1;p+1<s.size()&&result.size()<limit;p++){
  unsigned char c=(unsigned char)s[p];
  if(c!='\\'){if(c>=32)result.push_back((char)c);continue;}
  if(p+1>=s.size()-1)break;
  c=(unsigned char)s[++p];
  if(c=='n'||c=='r'||c=='t')c=' ';
  else if(c=='u'){
   if(p+4>=s.size()-1)break;
   unsigned cp=0;bool good=true;
   for(int i=1;i<=4;++i){int digit=hx(s[p+i]);if(digit<0){good=false;break;}cp=cp*16+(unsigned)digit;}
   p+=4;
   if(!good)continue;
   if(cp<=0x7f)c=(unsigned char)cp;
   else{
    if(cp<=0x7ff&&result.size()+2<=limit){
     result.push_back((char)(0xc0|(cp>>6)));
     result.push_back((char)(0x80|(cp&63)));
    }else if(cp>=0x800&&cp<0xd800&&result.size()+3<=limit){
     result.push_back((char)(0xe0|(cp>>12)));
     result.push_back((char)(0x80|((cp>>6)&63)));
     result.push_back((char)(0x80|(cp&63)));
    }
    continue;
   }
  }else if(c!='\\'&&c!='"'&&c!='/'&&c!='b'&&c!='f')continue;
  if(c>=32)result.push_back((char)c);
 }
 return result;
}
int integer(std::string_view s){return std::atoi(std::string(s).c_str());}
long longnum(std::string_view s){return std::atol(std::string(s).c_str());}
std::string stable_path(const std::string& id){return "/data/pstube/downloads/"+id+".mp4";}
bool save_text(const char* path,const std::string& payload){
 std::string temp=std::string(path)+".tmp";FILE* f=std::fopen(temp.c_str(),"wb");if(!f)return false;
 bool ok=std::fwrite(payload.data(),1,payload.size(),f)==payload.size()&&std::fflush(f)==0&&::fsync(fileno(f))==0;
 ok=std::fclose(f)==0&&ok;if(!ok){unlink(temp.c_str());return false;}
 if(rename(temp.c_str(),path)!=0){unlink(temp.c_str());return false;}return true;
}
std::string unescape(const std::string& s){
 std::string out;out.reserve(std::min<size_t>(s.size(),512));
 auto digit=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;return -1;};
 for(size_t i=0;i<s.size()&&out.size()<256;i++){
  if(s[i]=='%'&&i+2<s.size()&&digit(s[i+1])>=0&&digit(s[i+2])>=0){
   unsigned char c=(unsigned char)((digit(s[i+1])<<4)|digit(s[i+2]));i+=2;
   if(c>=32)out+=(char)c;
  }else if((unsigned char)s[i]>=32)out+=s[i];
 }return out;
}
std::string encode(const std::string& s){
 const char* h="0123456789ABCDEF";std::string o;
 for(unsigned char c:s){if(o.size()>950)break;
  if(std::isalnum(c)||c=='-'||c=='_'||c=='.')o+=(char)c;
  else {o+='%';o+=h[c>>4];o+=h[c&15];}
 }return o;
}

const char* kCatalogCache="/data/pstube/catalog-v1.txt";
bool save_feed_cache(const std::vector<Video>& videos){
 std::string data="PSTUBE-CATALOG:1\n";
 int limit=0;
 for(const auto& video:videos){
  if(!valid_id(video.id)||video.title.empty())continue;
  data+=video.id+"|"+encode(video.title)+"|"+encode(video.author)+"|"+
        std::to_string(std::max(0,video.length))+"|"+
        std::to_string(std::max(0L,video.views))+"\n";
  if(++limit>=60||data.size()>65000)break;
 }
 return save_text(kCatalogCache,data);
}
std::vector<Video> load_feed_cache(){
 std::vector<Video> videos;
 FILE* f=fopen(kCatalogCache,"rb");if(!f)return videos;
 char buffer[65536]{};
 size_t count=fread(buffer,1,sizeof(buffer)-1,f);
 bool oversized=!feof(f);fclose(f);
 std::string data(buffer,count);
 if(oversized||data.rfind("PSTUBE-CATALOG:1\n",0)!=0)return videos;
 size_t offset=17;
 while(offset<data.size()&&videos.size()<60){
  size_t end=data.find('\n',offset);if(end==std::string::npos)break;
  const std::string line=data.substr(offset,end-offset);offset=end+1;
  size_t a=line.find('|'),b=a==std::string::npos?a:line.find('|',a+1);
  size_t c=b==std::string::npos?b:line.find('|',b+1);
  size_t d=c==std::string::npos?c:line.find('|',c+1);
  if(a==std::string::npos||b==std::string::npos||c==std::string::npos||d==std::string::npos)continue;
  Video v;v.id=line.substr(0,a);
  if(!valid_id(v.id))continue;
  v.title=unescape(line.substr(a+1,b-a-1));
  v.author=unescape(line.substr(b+1,c-b-1));
  const std::string duration=line.substr(c+1,d-c-1),views=line.substr(d+1);
  auto numeric=[](const std::string& value)->bool{
    return !value.empty()&&value.size()<12&&
      std::all_of(value.begin(),value.end(),[](unsigned char ch){return std::isdigit(ch)!=0;});
  };
  if(v.title.empty()||!numeric(duration)||!numeric(views))continue;
  v.length=std::min(864000,std::atoi(duration.c_str()));
  v.views=std::atol(views.c_str());
  videos.push_back(std::move(v));
 }
 return videos;
}
void mkdirs(){mkdir("/data/pstube",0700);mkdir("/data/pstube/downloads",0700);}
struct Work {Service* svc;std::string query;std::string id;int kind;Video video;};
}
bool valid_id(const std::string& s){
 if(s.size()!=11)return false;
 for(char c:s)if(!std::isalnum((unsigned char)c)&&c!='_'&&c!='-')return false;
 return true;
}
std::string json_string(const std::string& obj,const std::string& name){return decode(field(obj,name));}
std::string escape_url(const std::string& s){
 const char* h="0123456789ABCDEF";std::string o;
 for(unsigned char c:s){if(std::isalnum(c)||c=='-'||c=='_'||c=='.'||c=='~')o+=(char)c;else {o+='%';o+=h[c>>4];o+=h[c&15];}}return o;
}
std::vector<Video> parse_search(const std::string& s){
 std::vector<Video> out;size_t p=0;skip(s,p);if(p==s.size()||s[p++]!='[')return out;
 while(p<s.size()&&out.size()<60){skip(s,p);if(s[p]==']')break;size_t e=value_end(s,p);if(e==p)break;auto v=std::string_view(s).substr(p,e-p);p=e;
  if(decode(field(v,"type"))=="video"||field(v,"type").empty()){Video a;a.id=decode(field(v,"videoId"));a.title=decode(field(v,"title"));a.author=decode(field(v,"author"));a.length=integer(field(v,"lengthSeconds"));a.views=longnum(field(v,"viewCount"));a.live=field(v,"liveNow")=="true";
   if(valid_id(a.id)&&!a.title.empty())out.push_back(std::move(a));
  }skip(s,p);if(p>=s.size()||s[p++]!=',')break;
 }return out;
}

int youtube_duration(const std::string& time){
 int count=0,seconds=0;std::string number;
 for(char c:time){
  if(c>='0'&&c<='9'){if(number.size()<4)number+=c;}
  else if(c==':'){
   if(number.empty())return 0;
   seconds=seconds*60+std::atoi(number.c_str());number.clear();++count;
  }else return 0;
 }
 if(number.empty()||count>2)return 0;
 seconds=seconds*60+std::atoi(number.c_str());
 return seconds>=0&&seconds<=864000?seconds:0;
}
std::string youtube_run(std::string_view obj){
 const auto runs=field(obj,"runs");size_t p=0;skip(runs,p);
 if(p>=runs.size()||runs[p++]!='[')return {};
 skip(runs,p);if(p>=runs.size()||runs[p]!='{')return {};
 const auto item=runs.substr(p,value_end(runs,p)-p);
 return decode(field(item,"text"));
}
std::vector<Video> parse_youtube_html(const std::string& html){
 std::vector<Video> videos;
 if(html.size()>kMaxJson)return videos;
 // The public, logged-out YouTube search page contains the renderer JSON.
 // Never interpret unrelated script data as a video: validate 11-char IDs.
 constexpr std::string_view needle="\"videoRenderer\":{";
 size_t from=html.find("ytInitialData");
 if(from==std::string::npos)from=html.find("\"videoRenderer\":{");
 if(from==std::string::npos)return videos;
 while(videos.size()<60){
  const size_t hit=html.find(needle,from);
  if(hit==std::string::npos)break;
  const size_t start=hit+needle.size()-1;
  const size_t end=value_end(html,start);
  if(end<=start||end>html.size()){from=hit+needle.size();continue;}
  const std::string_view item(html.data()+start,end-start);
  Video v;
  v.id=decode(field(item,"videoId"));
  v.title=youtube_run(field(item,"title"));
  if(v.title.empty())v.title=decode(field(field(item,"title"),"simpleText"));
  v.author=youtube_run(field(item,"longBylineText"));
  if(v.author.empty())v.author=youtube_run(field(item,"ownerText"));
  v.length=youtube_duration(decode(field(field(item,"lengthText"),"simpleText")));
  const std::string count=decode(field(field(item,"viewCountText"),"simpleText"));
  long views=0;
  for(char c:count){
   if(c>='0'&&c<='9'){if(views<=999999999L)views=views*10+(c-'0');}
   else if((c<'0'||c>'9')&&c!=','&&c!='.')break;
  }
  v.views=views;
  if(valid_id(v.id)&&!v.title.empty()&&
    std::none_of(videos.begin(),videos.end(),[&](const Video& old){return old.id==v.id;})){
   videos.push_back(std::move(v));
  }
  from=end;
 }
 return videos;
}
// Continuations come from the same live YouTube response as the video cards.
// Do not invent page tokens or use a fixed result offset.
std::string youtube_continuation(const std::string& json){
 size_t start=0;
 std::string result;
 constexpr std::string_view marker="\"continuationItemRenderer\":{";
 while((start=json.find(marker,start))!=std::string::npos){
  const size_t obj=start+marker.size()-1;
  const size_t end=value_end(json,obj);
  if(end<=obj||end>json.size()){start+=marker.size();continue;}
  const std::string_view item(json.data()+obj,end-obj);
  const auto endpoint=field(item,"continuationEndpoint");
  const auto command=field(endpoint,"continuationCommand");
  auto token=decode(field(command,"token"),4096);
  if(token.size()>=16&&token.size()<4096)result=std::move(token);
  start=end;
 }
 return result;
}
std::string youtube_client_version(const std::string& html){
 constexpr const char* keys[]={"INNERTUBE_CONTEXT_CLIENT_VERSION","clientVersion"};
 for(const char* key:keys){
  const std::string prefix=std::string("\"")+key+"\"";
  size_t p=html.find(prefix);
  if(p==std::string::npos)continue;
  p=html.find(':',p+prefix.size());if(p==std::string::npos)continue;
  ++p;skip(html,p);
  const auto v=decode(std::string_view(html).substr(p),80);
  if(v.size()>6&&v.size()<65&&
     std::all_of(v.begin(),v.end(),[](unsigned char c){return std::isalnum(c)||c=='.'||c=='-';}))
   return v;
 }
 return "2.20261010.00.00";
}
std::string continuation_body(const std::string& token,const std::string& version){
 // The tokens consist of web-safe characters; quoted text is properly escaped
 // to prevent malformed JSON or a request with an unintended command.
 auto quote=[](const std::string& value)->std::string{
  const char* digits="0123456789abcdef";std::string encoded="\"";
  for(unsigned char c:value){
   if(c=='"'||c=='\\'){encoded+='\\';encoded+=(char)c;}
   else if(c<32){encoded+="\\u00";encoded+=digits[c>>4];encoded+=digits[c&15];}
   else encoded+=(char)c;
  }
  return encoded+"\"";
 };
 return "{\"context\":{\"client\":{\"clientName\":\"WEB\",\"clientVersion\":"+
  quote(version)+",\"hl\":\"en\",\"gl\":\"US\"}},\"continuation\":"+quote(token)+"}";
}
Service::Service(){
 mkdirs();
 state_.videos=load_feed_cache();
 if(!state_.videos.empty()){
  state_.status="Showing last online results (cached); refreshing...";
  queue_thumbnails(state_.videos);
 }
 for(auto* path:{kSave,kFallback}){FILE* f=fopen(path,"rb");if(!f)continue;char buf[65536]{};size_t n=fread(buf,1,sizeof(buf)-1,f);fclose(f);std::string v(buf,n);if(v.substr(0,9)!="PSTUBE:1\n")continue;
  size_t pos=9;while(pos<v.size()){size_t e=v.find('\n',pos);if(e==std::string::npos)break;std::string line=v.substr(pos,e-pos);pos=e+1;
   if(line.substr(0,2)=="I "&&host_ok(line.substr(2)))instance_=line.substr(2);
   if(line.substr(0,2)=="F "&&valid_id(line.substr(2))&&fav_.size()<300)fav_.push_back(line.substr(2));
   if(line.substr(0,2)=="V "||line.substr(0,2)=="H "){
    size_t a=line.find('|',2),b=a==std::string::npos?a:line.find('|',a+1);
    if(a!=std::string::npos&&b!=std::string::npos){
     Video video;video.id=line.substr(2,a-2);
     video.title=unescape(line.substr(a+1,b-a-1));video.author=unescape(line.substr(b+1));
     if(valid_id(video.id)&&!video.title.empty()){
      auto& dst=line[0]=='V'?favorite_meta_:recent_meta_;
      if(dst.size()<100)dst.push_back(std::move(video));
     }
    }
   }
  }break;}
 DIR* d=opendir("/data/pstube/downloads");
 if(d){while(dirent* ent=readdir(d)){std::string name(ent->d_name);
   if(name.size()!=15||name.substr(11)!=".mp4")continue;
   auto id=name.substr(0,11);if(!valid_id(id))continue;
   state_.jobs.push_back(Job{id,id,100.0f,"Complete","",stable_path(id)});
  }closedir(d);}
}
Service::~Service(){while(busy_.load()||downloading_.load()||resolvingStream_.load())usleep(25000);}
bool Service::persist()const{
 std::string out="PSTUBE:1\nI "+instance_+"\n";for(const auto& v:fav_)out+="F "+v+"\n";
 for(const auto& v:favorite_meta_)if(valid_id(v.id))out+="V "+v.id+"|"+encode(v.title)+"|"+encode(v.author)+"\n";
 for(const auto& v:recent_meta_)if(valid_id(v.id))out+="H "+v.id+"|"+encode(v.title)+"|"+encode(v.author)+"\n";
 return save_text(kSave,out)||save_text(kFallback,out);
}
std::vector<std::string> Service::favourites()const{std::lock_guard<std::mutex> g(mutex_);return fav_;}
std::vector<Video> Service::favorite_videos()const{
 std::lock_guard<std::mutex> g(mutex_);std::vector<Video> out;
 for(const auto& id:fav_){auto it=std::find_if(favorite_meta_.begin(),favorite_meta_.end(),[&](const Video& v){return v.id==id;});
  out.push_back(it!=favorite_meta_.end()?*it:Video{id,id,"Saved favourite"});}
 return out;
}
std::vector<Video> Service::history_videos()const{
 std::lock_guard<std::mutex> g(mutex_);return recent_meta_;
}
void Service::record_history(const Video& video){
 if(!valid_id(video.id))return;std::lock_guard<std::mutex> g(mutex_);
 recent_meta_.erase(std::remove_if(recent_meta_.begin(),recent_meta_.end(),[&](const Video& v){return v.id==video.id;}),recent_meta_.end());
 recent_meta_.insert(recent_meta_.begin(),video);
 if(recent_meta_.size()>50)recent_meta_.resize(50);
 persist();
}
bool Service::toggle_favourite(const Video& video){
 if(!valid_id(video.id))return false;std::lock_guard<std::mutex> g(mutex_);
 auto i=std::find(fav_.begin(),fav_.end(),video.id);
 favorite_meta_.erase(std::remove_if(favorite_meta_.begin(),favorite_meta_.end(),[&](const Video& v){return v.id==video.id;}),favorite_meta_.end());
 if(i!=fav_.end())fav_.erase(i);
 else{if(fav_.size()>=100)return false;fav_.insert(fav_.begin(),video.id);favorite_meta_.insert(favorite_meta_.begin(),video);}
 return persist();
}

bool Service::is_favourite(const std::string& id)const{std::lock_guard<std::mutex> g(mutex_);return std::find(fav_.begin(),fav_.end(),id)!=fav_.end();}
bool Service::toggle_favourite(const std::string& id){
 if(!valid_id(id))return false;std::lock_guard<std::mutex> g(mutex_);
 auto it=std::find(fav_.begin(),fav_.end(),id);if(it==fav_.end()){if(fav_.size()>=300)return false;fav_.push_back(id);}else fav_.erase(it);return persist();
}
void Service::set_instance(const std::string& s){if(!host_ok(s))return;std::lock_guard<std::mutex> g(mutex_);instance_=s;persist();}
std::string Service::instance()const{std::lock_guard<std::mutex> g(mutex_);return instance_;}
Snapshot Service::snapshot()const{
 std::lock_guard<std::mutex> g(mutex_);
 Snapshot copy=state_;
 for(auto& j:copy.jobs)if(j.id==downloading_id_&&j.state=="Downloading")j.progress=(float)download_progress_.load();
 return copy;
}
std::string Service::local_path(const std::string& id)const {
 if(!valid_id(id))return {};
 const std::string path=stable_path(id);
 struct stat st{};return stat(path.c_str(),&st)==0&&st.st_size>1024?path:std::string{};
}
std::string pick_mp4(const std::string& json){
 const auto arr=field(json,"formatStreams");size_t p=0;skip(arr,p);
 if(p>=arr.size()||arr[p++]!='[')return {};
 std::string winner;int quality=-1;
 while(p<arr.size()){
  skip(arr,p);if(p>=arr.size()||arr[p]==']')break;
  size_t e=value_end(arr,p);if(e<=p)break;auto item=arr.substr(p,e-p);
  std::string container=decode(field(item,"container"));
  std::string mime=decode(field(item,"type"));
  int resolution=integer(field(item,"qualityLabel"));
  const std::string quality_text=decode(field(item,"qualityLabel"));
  if(quality_text.size())resolution=std::atoi(quality_text.c_str());
  std::string url=decode(field(item,"url"),12000);
  if((container=="mp4"||mime.find("video/mp4")!=std::string::npos)&&url.rfind("https://",0)==0&&resolution<=1080&&resolution>quality){
   winner=url;quality=resolution;
  }
  p=e;skip(arr,p);if(p>=arr.size()||arr[p++]!=',')break;
 }
 return winner;
}

bool Service::trending(){return search("@@TRENDING@@");}
bool Service::search(const std::string& query){
 if(query.empty()||query.size()>100||busy_.exchange(true))return false;
 {std::lock_guard<std::mutex> g(mutex_);
   currentQuery_=query;continuation_.clear();pagingProvider_.clear();clientVersion_.clear();providerPage_=1;
   ++feedGeneration_;
   state_.searching=true;state_.loadingMore=false;state_.hasMore=false;
   state_.status="Contacting video providers...";
 }
 auto* w=new Work{this,query,"",0,{}};
 pthread_t th{};pthread_attr_t a;pthread_attr_init(&a);pthread_attr_setstacksize(&a,1024*1024);
 int rc=pthread_create(&th,&a,[](void* p)->void* {
  std::unique_ptr<Work> work((Work*)p);Service& svc=*work->svc;
  const bool trending=work->query=="@@TRENDING@@";
  const std::string directQuery=trending?"PS5 gaming":work->query;
  std::string youtubeHtml,youtubeError;
  const bool youtubeOk=https("www.youtube.com",
     "/results?search_query="+escape_url(directQuery),youtubeHtml,youtubeError);
  std::vector<Video> directVideos=youtubeOk?parse_youtube_html(youtubeHtml):std::vector<Video>{};
  if(!directVideos.empty()){
   save_feed_cache(directVideos);
   queue_thumbnails(directVideos);
   const std::string next=youtube_continuation(youtubeHtml);
   const std::string version=youtube_client_version(youtubeHtml);
   {
    std::lock_guard<std::mutex> g(svc.mutex_);
    svc.continuation_=next;svc.clientVersion_=version;
    svc.pagingProvider_.clear();
    svc.state_.videos=std::move(directVideos);
    svc.state_.hasMore=!next.empty();
    svc.state_.status="YouTube Search: "+std::to_string(svc.state_.videos.size())+
       " videos"+(next.empty()?" (no continuation available)":" - scroll down to load more");
    svc.state_.searching=false;
   }
   svc.busy_.store(false,std::memory_order_release);
   return nullptr;
  }

  const std::string path=trending?"/api/v1/trending?region=US":
     "/api/v1/search?q="+escape_url(work->query)+"&type=video";
  // Trusted instances from the official Invidious public-instance directory.
  // Always try the user-selected instance first, without silently changing their preference.
  const std::string candidates[]={
    svc.instance(),"inv.nadeko.net","invidious.nerdvpn.de",
    "yt.chocolatemoo53.com","invidious.tiekoetter.com"};
  std::vector<Video> videos;
  std::string lastError=youtubeOk?"YouTube HTML contained no recognizable videos":"YouTube: "+youtubeError,successful;
  for(size_t index=0;index<sizeof(candidates)/sizeof(candidates[0]);++index){
   const std::string& host=candidates[index];
   bool duplicate=false;
   for(size_t earlier=0;earlier<index;++earlier)if(candidates[earlier]==host)duplicate=true;
   if(duplicate)continue;
   {
    std::lock_guard<std::mutex> g(svc.mutex_);
    svc.state_.status="Contacting "+host+"...";
   }
   std::string response,error;
   if(!https(host,path,response,error)){lastError=host+": "+error;continue;}
   videos=parse_search(response);
   if(!videos.empty()){successful=host;break;}
   // An empty array can be a valid response for a rare search term.
   // Retry another instance before showing no results.
   const bool emptyArray=response.find('[')!=std::string::npos&&
      response.find(']')!=std::string::npos;
   lastError=host+(emptyArray?": no matching videos":": invalid API response");
  }
  const bool found=!videos.empty();
  if(found){save_feed_cache(videos);queue_thumbnails(videos);}
  {
   std::lock_guard<std::mutex> g(svc.mutex_);
   if(found){svc.state_.videos=std::move(videos);svc.pagingProvider_=successful;
     svc.state_.hasMore=svc.state_.videos.size()>=10;}
   svc.state_.status=found?"Loaded "+std::to_string(svc.state_.videos.size())+
      " videos via "+successful:
      ("No videos from any provider. Last error: "+lastError);
   svc.state_.searching=false;
  }
  svc.busy_.store(false,std::memory_order_release);return nullptr;
 },w);
 pthread_attr_destroy(&a);
 if(rc){delete w;busy_.store(false);std::lock_guard<std::mutex> g(mutex_);state_.searching=false;state_.status="Unable to start network thread";return false;}
 pthread_detach(th);return true;
}
bool Service::load_more(){
 if(busy_.exchange(true))return false;
 std::string token,version,query,host;
 int page=0;
 {
  std::lock_guard<std::mutex> g(mutex_);
  if(!state_.hasMore||state_.searching||state_.loadingMore){
   busy_=false;return false;
  }
  token=continuation_;version=clientVersion_;query=currentQuery_;
  host=pagingProvider_;page=providerPage_+1;
  if(token.empty()&&host.empty()){state_.hasMore=false;busy_=false;return false;}
  state_.loadingMore=true;
  state_.status="Loading more videos...";
 }
 struct More {Service* svc;std::string token,version,query,host;int page;};
 auto* w=new More{this,token,version,query,host,page};
 pthread_t th{};pthread_attr_t attr;pthread_attr_init(&attr);
 pthread_attr_setstacksize(&attr,1024*1024);
 const int rc=pthread_create(&th,&attr,[](void* arg)->void* {
  std::unique_ptr<More> w((More*)arg);Service& svc=*w->svc;
  std::string response,error;
  bool ok=false;std::string next;
  std::vector<Video> added;
  if(!w->token.empty()){
   ok=https("www.youtube.com","/youtubei/v1/search?prettyPrint=false",
     response,error,continuation_body(w->token,w->version));
   if(ok){added=parse_youtube_html(response);next=youtube_continuation(response);}
  }else if(!w->host.empty()){
   const bool trending=w->query=="@@TRENDING@@";
   const std::string path=trending?
      "/api/v1/trending?region=US&page="+std::to_string(w->page):
      "/api/v1/search?q="+escape_url(w->query)+
        "&type=video&page="+std::to_string(w->page);
   ok=https(w->host,path,response,error);
   if(ok)added=parse_search(response);
  }
  if(ok&& !added.empty())queue_thumbnails(added);
  {
   std::lock_guard<std::mutex> g(svc.mutex_);
   size_t newCount=0;
   for(const auto& v:added){
    if(svc.state_.videos.size()>=1800)break;
    const auto& list=svc.state_.videos;
    if(std::none_of(list.begin(),list.end(),[&](const Video& existing){return existing.id==v.id;})){
     svc.state_.videos.push_back(v);newCount++;
    }
   }
   if(!w->token.empty())svc.continuation_=next;
   else if(ok)svc.providerPage_=w->page;
   // Do not spin repeatedly on a failing or duplicate page.
   svc.state_.hasMore=ok&&newCount>0&&svc.state_.videos.size()<1800&&
     (!w->token.empty()?!next.empty():added.size()>=8);
   svc.state_.status=!ok?"Couldn't load the next page: "+error:
     newCount?std::to_string(svc.state_.videos.size())+" videos - scroll for more":
              "End of available search results";
   svc.state_.loadingMore=false;
  }
  svc.busy_.store(false,std::memory_order_release);return nullptr;
 },w);
 pthread_attr_destroy(&attr);
 if(rc){
  delete w;busy_=false;
  std::lock_guard<std::mutex> g(mutex_);
  state_.loadingMore=false;state_.status="Unable to start paging worker";return false;
 }
 pthread_detach(th);return true;
}
bool Service::details(const std::string& id){return valid_id(id);}

std::string pick_youtube_video_mp4(const std::string& html,std::string& reason){
 const std::string key="ytInitialPlayerResponse = ";
 auto off=html.find(key);
 if(off==std::string::npos){reason="YouTube returned no playback metadata";return {};}
 off=html.find('{',off+key.size());
 if(off==std::string::npos){reason="Playback metadata malformed";return {};}
 const auto end=value_end(html,off);
 if(end<=off||end>html.size()){reason="Playback metadata truncated";return {};}
 const std::string_view player(html.data()+off,end-off);
 const std::string status=decode(field(field(player,"playabilityStatus"),"status"));
 if(!status.empty()&&status!="OK"){
  reason="YouTube player: "+status;
  const std::string text=decode(field(field(player,"playabilityStatus"),"reason"));
  if(!text.empty())reason+=" ("+text.substr(0,100)+")";
  return {};
 }
 const auto streaming=field(player,"streamingData");
 const auto list=field(streaming,"formats");
 size_t p=0;skip(list,p);
 if(p==list.size()||list[p++]!='['){reason="No progressive audio+video stream available";return {};}
 int best=-1;std::string url;
 bool signedOnly=false;
 while(p<list.size()){
  skip(list,p);if(p>=list.size()||list[p]==']')break;
  const auto e=value_end(list,p);if(e<=p)break;
  const auto item=list.substr(p,e-p);p=e;
  const std::string mime=decode(field(item,"mimeType"));
  const int res=std::atoi(decode(field(item,"qualityLabel")).c_str());
  const std::string candidate=decode(field(item,"url"),12000);
  if(mime.find("video/mp4")!=std::string::npos && res>=best && res<=1080){
   if(candidate.rfind("https://",0)==0){url=candidate;best=res;}
   else if(!field(item,"signatureCipher").empty())signedOnly=true;
  }
  skip(list,p);if(p>=list.size()||list[p++]!=',')break;
 }
 if(!url.empty())return url;
 reason=signedOnly?"YouTube stream requires signature deciphering (not available on-console)":
     "No compatible unprotected MP4 stream found";
 return {};
}

bool resolve_android_mp4(const std::string& id,std::string& mediaUrl,std::string& error){
 mediaUrl.clear();error.clear();
 if(!valid_id(id)){error="Invalid video identifier";return false;}
 // Only public logged-out playback contexts are used, no cookies or account tokens.
 // YouTube's Android client currently provides itag 18 combined H.264/AAC on
 // some videos without JS deciphering. PO-token/login-locked videos remain unavailable.
 const std::string agent=
  "com.google.android.youtube/21.26.364 (Linux; U; Android 11) gzip";
 const std::string body=
  "{\"videoId\":\""+id+"\",\"context\":{\"client\":{\"clientName\":\"ANDROID\","
  "\"clientVersion\":\"21.26.364\",\"hl\":\"en\",\"gl\":\"US\","
  "\"androidSdkVersion\":30,\"osName\":\"Android\",\"osVersion\":\"11\","
  "\"userAgent\":\""+agent+"\"}},"
  "\"playbackContext\":{\"contentPlaybackContext\":{\"html5Preference\":\"HTML5_PREF_WANTS\"}}}";
 std::string response;
 if(!https("www.youtube.com","/youtubei/v1/player?prettyPrint=false",response,error,body)){
  error="Android player API: "+error;return false;
 }
 std::string status=decode(field(field(response,"playabilityStatus"),"status"));
 if(status!="OK"){
  const std::string why=decode(field(field(response,"playabilityStatus"),"reason"));
  error="Android player: "+(status.empty()?"no playable response":status);
  if(!why.empty())error+=" ("+why.substr(0,120)+")";
  return false;
 }
 const auto stream=field(response,"streamingData");
 const auto formats=field(stream,"formats");
 size_t pos=0;skip(formats,pos);
 if(pos>=formats.size()||formats[pos++]!='['){
  error="Android player: no combined video/audio formats";return false;
 }
 std::string candidate;int best=-1;bool requiredCipher=false;
 while(pos<formats.size()){
  skip(formats,pos);if(pos>=formats.size()||formats[pos]==']')break;
  const size_t end=value_end(formats,pos);if(end<=pos)break;
  const auto item=formats.substr(pos,end-pos);pos=end;
  const std::string mime=decode(field(item,"mimeType"));
  const int itag=integer(field(item,"itag"));
  const int height=integer(field(item,"height"));
  const int quality=std::atoi(decode(field(item,"qualityLabel")).c_str());
  const int resolution=height>0?height:quality;
  const std::string url=decode(field(item,"url"),12000);
  if(mime.find("video/mp4")!=std::string::npos && (itag==18||resolution<=1080)){
   if(url.rfind("https://",0)==0&&resolution>=best){
    // Do not pass redirect-to-HTML links to FFmpeg.
    candidate=url;best=resolution;
   }else if(!field(item,"signatureCipher").empty())requiredCipher=true;
  }
  skip(formats,pos);if(pos>=formats.size()||formats[pos++]!=',')break;
 }
 if(candidate.empty()){
  error=requiredCipher?"Android player stream requires deciphering":
   "Android player did not provide a direct MP4 URL";
  return false;
 }
 mediaUrl=std::move(candidate);
 return true;
}
void Service::cancel_stream(){
 std::lock_guard<std::mutex> g(mutex_);
 state_.streamId.clear();state_.streamUrl.clear();state_.streamError.clear();
 state_.streamResolving=false;
}
bool Service::start_stream(const Video& video){
 if(!valid_id(video.id)||resolvingStream_.exchange(true))return false;
 {
  std::lock_guard<std::mutex> g(mutex_);
  state_.streamId=video.id;
  state_.streamUrl.clear();state_.streamError.clear();state_.streamResolving=true;
 }
 auto* work=new Work{this,"",video.id,3,video};
 pthread_t thread{};pthread_attr_t attr;
 pthread_attr_init(&attr);pthread_attr_setstacksize(&attr,1024*1024);
 const int rc=pthread_create(&thread,&attr,[](void* ptr)->void*{
  std::unique_ptr<Work> w((Work*)ptr);Service& svc=*w->svc;
  std::string url,error;
  if(!resolve_android_mp4(w->id,url,error)){
   const std::string androidError=error;
   const std::string host=svc.instance();
   std::string page,webError;
   if(https("www.youtube.com","/watch?v="+w->id,page,webError))
    url=pick_youtube_video_mp4(page,webError);
   if(url.empty()){
    std::string response,apiError;
    if(https(host,"/api/v1/videos/"+w->id,response,apiError))
     url=pick_mp4(response);
    if(url.empty())
     error=androidError+"; Web: "+webError+"; Invidious: "+apiError;
   }
  }
  // A stream URL is kept only in volatile memory and discarded when leaving
  // the player. Do not log signed query parameters.
  {
   std::lock_guard<std::mutex> g(svc.mutex_);
   if(svc.state_.streamId==w->id){
    svc.state_.streamUrl=std::move(url);
    svc.state_.streamError=svc.state_.streamUrl.empty()?
      (error.empty()?"No direct HTTPS stream available":error):std::string();
    svc.state_.streamResolving=false;
   }
  }
  svc.resolvingStream_.store(false,std::memory_order_release);return nullptr;
 },work);
 pthread_attr_destroy(&attr);
 if(rc){
  delete work;resolvingStream_=false;
  std::lock_guard<std::mutex> g(mutex_);
  state_.streamResolving=false;state_.streamError="Could not start stream resolver";
  return false;
 }
 pthread_detach(thread);return true;
}
bool Service::download(const Video& video){
 if(!valid_id(video.id)||downloading_.exchange(true))return false;
 const std::string path=local_path(video.id);
 if(!path.empty()){std::lock_guard<std::mutex> g(mutex_);state_.status="Already downloaded";downloading_=false;return true;}
 {
  std::lock_guard<std::mutex> g(mutex_);
  downloading_id_=video.id;download_progress_=0;
  state_.status="Resolving MP4 format...";
  state_.jobs.push_back(Job{video.id,video.title,0.0f,"Resolving","",stable_path(video.id)});
 }
 auto* work=new Work{this,"",video.id,1,video};
 pthread_t th{};pthread_attr_t attr;pthread_attr_init(&attr);pthread_attr_setstacksize(&attr,1024*1024);
 int rc=pthread_create(&th,&attr,[](void* ptr)->void*{
  std::unique_ptr<Work> w((Work*)ptr);Service& svc=*w->svc;
  std::string body,error,host=svc.instance();
  std::string url;
  bool ok=resolve_android_mp4(w->id,url,error);
  if(!ok){
   const std::string androidError=error;
   std::string watchError,watchHtml;
   const bool watchOK=https("www.youtube.com","/watch?v="+w->id,watchHtml,watchError);
   if(watchOK)url=pick_youtube_video_mp4(watchHtml,watchError);
   if(url.empty()){
    std::string response,apiError;
    const bool apiOK=https(host,"/api/v1/videos/"+w->id,response,apiError);
    if(apiOK)url=pick_mp4(response);
    if(url.empty()){
     error=androidError+"; Web: "+(watchError.empty()?"no MP4":watchError)+
       "; Invidious: "+(apiOK?"no MP4":apiError);
    }
   }
  }
  ok=!url.empty();
  if(ok){
   {std::lock_guard<std::mutex> g(svc.mutex_);for(auto& j:svc.state_.jobs)if(j.id==w->id)j.state="Downloading";}
   ok=fetch_mp4(url,stable_path(w->id),svc.download_progress_,error);
  }
  {
   std::lock_guard<std::mutex> g(svc.mutex_);
   for(auto& j:svc.state_.jobs)if(j.id==w->id){
    j.progress=(float)svc.download_progress_.load();j.state=ok?"Complete":"Failed";j.error=error;
   }
   svc.state_.status=ok?"Download complete: "+w->video.title:(error.empty()?"Download failed":error);
   svc.downloading_id_.clear();
  }
  svc.downloading_=false;return nullptr;
 },work);
 pthread_attr_destroy(&attr);
 if(rc){delete work;downloading_=false;std::lock_guard<std::mutex> g(mutex_);state_.status="Could not launch download worker";return false;}
 pthread_detach(th);return true;
}
} // namespace pstube
