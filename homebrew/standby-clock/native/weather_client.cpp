// SPDX-License-Identifier: MIT
// PS5 weather over validated HTTPS: IP-area lookup + current Open-Meteo readings.
// No API keys, fabricated values, blocked draw calls or PC proxy.
#include "weather_client.hpp"
#include "ca_bundle.inc"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <pthread.h>
#include <string>
#include <string_view>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctime>
namespace standby::weather {
namespace {
std::mutex guard;
std::atomic<bool> busy{false};
Data current;
Location saved_location;
void note(const char* m){
 mkdir("/data/StandbyClock",0755);
 if(FILE* f=fopen("/data/StandbyClock/clock.log","ab")){
  fprintf(f,"[WEATHER v0.1.1] %s\n",m);fclose(f);
 }
}
struct Https {
 SSL_CTX* ctx=nullptr;
 SSL* ssl=nullptr;
 int fd=-1;
 ~Https(){if(ssl)SSL_free(ssl);if(fd>=0)close(fd);if(ctx)SSL_CTX_free(ctx);}
 bool connect_to(const char* host,std::string& err){
  ctx=SSL_CTX_new(TLS_client_method());
  if(!ctx){err="TLS INIT";return false;}
  SSL_CTX_set_min_proto_version(ctx,TLS1_2_VERSION);
  SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,nullptr);
  BIO* input=BIO_new_mem_buf(kCaBundle,-1);
  if(!input){err="NO TRUST ROOTS";return false;}
  X509_STORE* store=SSL_CTX_get_cert_store(ctx);
  int count=0;
  while(X509* cert=PEM_read_bio_X509(input,nullptr,nullptr,nullptr)){
   if(X509_STORE_add_cert(store,cert)==1)count++;
   X509_free(cert);
  }
  BIO_free(input);ERR_clear_error();
  if(!count){err="BAD CA BUNDLE";return false;}
  addrinfo hints{};hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;
  addrinfo* list=nullptr;
  if(getaddrinfo(host,"443",&hints,&list)!=0){err="DNS";return false;}
  for(addrinfo* a=list;a;a=a->ai_next){
   int candidate=socket(a->ai_family,a->ai_socktype,a->ai_protocol);
   if(candidate<0)continue;
   timeval timeout{8,0};
   setsockopt(candidate,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
   setsockopt(candidate,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
   if(::connect(candidate,a->ai_addr,a->ai_addrlen)==0){fd=candidate;break;}
   close(candidate);
  }
  freeaddrinfo(list);
  if(fd<0){err="CONNECT";return false;}
  ssl=SSL_new(ctx);
  if(!ssl){err="SSL NEW";return false;}
  SSL_set_fd(ssl,fd);
  SSL_set_tlsext_host_name(ssl,host);
  SSL_set1_host(ssl,host);
  if(SSL_connect(ssl)!=1||SSL_get_verify_result(ssl)!=X509_V_OK){
   err="TLS VERIFY";return false;
  }
  return true;
 }
};
bool decode_chunked(std::string_view input,std::string& output){
 size_t i=0;
 while(i<input.size()){
  size_t end=input.find("\r\n",i);
  if(end==std::string_view::npos||end-i>48)return false;
  std::string hex(input.substr(i,end-i));
  size_t semi=hex.find(';');if(semi!=std::string::npos)hex.resize(semi);
  char* stop=nullptr;unsigned long length=strtoul(hex.c_str(),&stop,16);
  if(stop==hex.c_str()||*stop||length>65536)return false;
  i=end+2;
  if(!length)return true;
  if(length>input.size()-i||output.size()+length>65536)return false;
  output.append(input.data()+i,length);
  i+=length;
  if(i+2>input.size()||input.substr(i,2)!="\r\n")return false;
  i+=2;
 }
 return false;
}
bool https_get(const char* host,const std::string& path,std::string& body,std::string& error){
 Https c;if(!c.connect_to(host,error))return false;
 std::string request="GET "+path+" HTTP/1.1\r\nHost: "+host+
    "\r\nUser-Agent: StandbyClock-PS5/0.1.1\r\nAccept: application/json\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n";
 for(size_t pos=0;pos<request.size();){
  int n=SSL_write(c.ssl,request.data()+pos,int(request.size()-pos));
  if(n<=0){error="SEND";return false;}pos+=(size_t)n;
 }
 std::string raw;raw.reserve(4096);
 char chunk[4096];
 for(;;){
  int n=SSL_read(c.ssl,chunk,sizeof(chunk));
  if(n<=0)break;
  if(raw.size()+(size_t)n>65536){error="TOO LARGE";return false;}
  raw.append(chunk,(size_t)n);
 }
 size_t split=raw.find("\r\n\r\n");
 if(split==std::string::npos){error="HTTP HEADERS";return false;}
 int status=0;
 if(sscanf(raw.c_str(),"HTTP/%*s %d",&status)!=1){error="HTTP STATUS";return false;}
 if(status!=200){error="HTTP "+std::to_string(status);return false;}
 std::string header=raw.substr(0,split);
 std::transform(header.begin(),header.end(),header.begin(),[](unsigned char ch){return (char)std::tolower(ch);});
 const std::string_view rest(raw.data()+split+4,raw.size()-split-4);
 if(header.find("transfer-encoding: chunked")!=std::string::npos){
  if(!decode_chunked(rest,body)){error="CHUNKED";return false;}
 }else{
  body.assign(rest);
  size_t p=header.find("content-length:");
  if(p!=std::string::npos){
   unsigned long expected=strtoul(header.c_str()+p+15,nullptr,10);
   if(expected!=body.size()){error="TRUNCATED";return false;}
  }
 }
 return !body.empty();
}
void cache_location(const Location& loc){
 FILE* f=fopen("/data/StandbyClock/weather-location.tmp","wb");if(!f)return;
 fprintf(f,"%.5f %.5f\n%s\n",loc.latitude,loc.longitude,loc.city.c_str());
 if(fclose(f)==0)rename("/data/StandbyClock/weather-location.tmp","/data/StandbyClock/weather-location.txt");
}
void cache_weather(const Data& data){
 FILE* f=fopen("/data/StandbyClock/weather.tmp","wb");if(!f)return;
 fprintf(f,"%d %d %d %d %lld\n%s\n",
  data.temperature,data.code,data.humidity,data.windKmh,(long long)time(nullptr),data.city.c_str());
 if(fclose(f)==0)rename("/data/StandbyClock/weather.tmp","/data/StandbyClock/weather.txt");
}
void load_cache(){
 char city[64]={};
 double lat=0,lon=0;
 if(FILE* f=fopen("/data/StandbyClock/weather-location.txt","rb")){
  if(fscanf(f,"%lf %lf\n",&lat,&lon)==2 && fgets(city,sizeof(city),f)){
   city[strcspn(city,"\r\n")]=0;
   if(lat>=-90&&lat<=90&&lon>=-180&&lon<=180&&strlen(city)<30)
    saved_location={true,lat,lon,city};
  }fclose(f);
 }
 int temp=0,code=0,hum=0,wind=0;
 long long ts=0;
 memset(city,0,sizeof(city));
 if(FILE* f=fopen("/data/StandbyClock/weather.txt","rb")){
  if(fscanf(f,"%d %d %d %d %lld\n",&temp,&code,&hum,&wind,&ts)==5&&fgets(city,sizeof(city),f)){
   city[strcspn(city,"\r\n")]=0;
   if(temp>=-110&&temp<=70&&code>=0&&code<=99&&hum>=0&&hum<=100&&wind>=0&&wind<=400&&
      ts>0&&time(nullptr)>=time_t(ts) && time(nullptr)-time_t(ts)<86400 && strlen(city)<30){
    current.valid=true;current.stale=true;
    current.temperature=temp;current.code=code;current.humidity=hum;current.windKmh=wind;
    current.city=city;current.condition=condition(code);current.status="CACHED WEATHER";
   }
  }fclose(f);
 }
}
void* worker(void*){
 std::string json,error;
 Location loc;
 if(https_get("ipapi.co","/json/",json,error)&&parse_location(json,loc)){
  {std::lock_guard<std::mutex> lock(guard);saved_location=loc;}
  cache_location(loc);
  note("IP AREA DETECTED");
 }else{
  std::lock_guard<std::mutex> lock(guard);
  loc=saved_location;
  note("IP AREA FALLBACK TO CACHED LOCATION");
 }
 bool success=false;Data parsed;
 if(loc.valid){
  char path[290];
  snprintf(path,sizeof(path),
    "/v1/forecast?latitude=%.5f&longitude=%.5f&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m&temperature_unit=celsius&wind_speed_unit=kmh&forecast_days=1",
    loc.latitude,loc.longitude);
  json.clear();error.clear();
  if(https_get("api.open-meteo.com",path,json,error)&&parse_current(json,parsed)){
   parsed.city=loc.city;
   success=true;
  }else if(error.empty())error="INVALID FORECAST";
 }else error="LOCATION UNAVAILABLE";
 {
  std::lock_guard<std::mutex> lock(guard);
  if(success){current=parsed;current.status="LIVE WEATHER";current.busy=false;}
  else{
   current.stale=current.valid;
   current.status=error.empty()?"WEATHER OFFLINE":error;
   current.busy=false;
  }
 }
 if(success){cache_weather(parsed);note("CURRENT WEATHER OK");}
 else note("WEATHER FETCH FAILED");
 busy.store(false,std::memory_order_release);
 return nullptr;
}
} // namespace
void init(){
 mkdir("/data/StandbyClock",0755);
 std::lock_guard<std::mutex> lock(guard);
 load_cache();
}
bool refresh(){
 bool expected=false;
 if(!busy.compare_exchange_strong(expected,true))return false;
 {
  std::lock_guard<std::mutex> lock(guard);
  current.busy=true;current.status="UPDATING WEATHER";
 }
 pthread_attr_t attrs;pthread_attr_init(&attrs);
 pthread_attr_setstacksize(&attrs,1024*1024);
 pthread_t th{};
 int result=pthread_create(&th,&attrs,worker,nullptr);
 pthread_attr_destroy(&attrs);
 if(result!=0){
  std::lock_guard<std::mutex> lock(guard);
  busy.store(false);current.busy=false;current.status="WEATHER WORKER FAILED";
  note("WEATHER THREAD FAILED");
  return false;
 }
 pthread_detach(th);
 return true;
}
Data snapshot(){
 std::lock_guard<std::mutex> lock(guard);
 return current;
}
} // namespace standby::weather
