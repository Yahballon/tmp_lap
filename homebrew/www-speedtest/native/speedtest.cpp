// SPDX-License-Identifier: GPL-3.0-or-later
// Cloudflare edge speed test over verified TLS; no external service, no mock readings.
#include "speedtest.hpp"
#include "ca_bundle.inc"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
namespace speedtest {
namespace {
std::mutex guard;
Snapshot state;
std::atomic<bool> busy{false};
std::atomic<int> chosen{0};
static const Server serverList[]={
 {"Cloudflare Automatic","GLOBAL","speed.cloudflare.com",""},
 {"Frankfurt, Germany","DE","fra.speedtest.clouvider.net","/backend"},
 {"Amsterdam, Netherlands","NL","ams.speedtest.clouvider.net","/backend"},
 {"London, United Kingdom","GB","lon.speedtest.clouvider.net","/backend"},
 {"New York, USA","US","nyc.speedtest.clouvider.net","/backend"},
 {"Los Angeles, USA","US","la.speedtest.clouvider.net","/backend"},
 {"Prague, Czechia","CZ","speedtest.cesnet.cz","/backend"},
 {"Rome, Italy","IT","st-be-rm2.infra.garr.it",""},
 {"Tokyo, Japan","JP","librespeed.a573.net","/backend"},
 {"Singapore","SG","speedtest.dsgroupmedia.com","/backend"},
 {"Chicago, USA","US","chispeed.sharktech.net","/backend"},
 {"Amsterdam Sharktech","NL","amsspeed.sharktech.net","/backend"}
};
static constexpr int count=(int)(sizeof(serverList)/sizeof(serverList[0]));
static const Server& server(){return serverList[chosen.load()];}
using Clock=std::chrono::steady_clock;
double elapsed(Clock::time_point t){return std::chrono::duration<double>(Clock::now()-t).count();}
void update(int phase,const char* message,double progress) {
 std::lock_guard<std::mutex> lock(guard);
 state.phase=phase;state.status=message;state.progress=progress;
}
struct Connection {
 SSL_CTX* ctx=nullptr; SSL* ssl=nullptr;int fd=-1;
 ~Connection(){if(ssl){SSL_shutdown(ssl);SSL_free(ssl);}if(fd>=0)close(fd);if(ctx)SSL_CTX_free(ctx);}
 bool open(std::string& err) {
  const char* host=server().host;
  ctx=SSL_CTX_new(TLS_client_method());
  if(!ctx){err="Could not initialize TLS";return false;}
  SSL_CTX_set_min_proto_version(ctx,TLS1_2_VERSION);
  SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,nullptr);
  BIO* input=BIO_new_mem_buf(kCaBundle,-1);
  if(!input){err="Missing certificate data";return false;}
  X509_STORE* store=SSL_CTX_get_cert_store(ctx);int trusted=0;
  while(X509* x=PEM_read_bio_X509(input,nullptr,nullptr,nullptr)){
   if(X509_STORE_add_cert(store,x)==1)trusted++;
   X509_free(x);
  }
  BIO_free(input);ERR_clear_error();
  if(!trusted){err="Certificate bundle invalid";return false;}
  addrinfo hints{};hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;
  addrinfo* addresses=nullptr;
  if(getaddrinfo(host,"443",&hints,&addresses)!=0){err="DNS lookup failed";return false;}
  for(addrinfo* a=addresses;a;a=a->ai_next){
   fd=socket(a->ai_family,a->ai_socktype,a->ai_protocol);
   if(fd<0)continue;
   timeval tv{8,0};
   setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
   setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
   if(connect(fd,a->ai_addr,a->ai_addrlen)==0)break;
   close(fd);fd=-1;
  }
  freeaddrinfo(addresses);
  if(fd<0){err="Could not connect to Cloudflare";return false;}
  ssl=SSL_new(ctx);
  if(!ssl){err="TLS connection allocation failed";return false;}
  SSL_set_fd(ssl,fd);SSL_set_tlsext_host_name(ssl,host);SSL_set1_host(ssl,host);
  if(SSL_connect(ssl)!=1||SSL_get_verify_result(ssl)!=X509_V_OK){
    err="TLS handshake or hostname verification failed";return false;
  }
  return true;
 }
 bool writeAll(const char* data,size_t size) {
  while(size){int n=SSL_write(ssl,data,(int)(size>16384?16384:size));if(n<=0)return false;data+=n;size-=(size_t)n;}
  return true;
 }
};
// Parse HTTP headers before counting payload. Stop on response size limits;
// using Connection:close to avoid pooled response ambiguity.
bool request(bool upload,size_t bytes,double& seconds,std::string& error) {
 Connection c;if(!c.open(error))return false;
 const bool cf=chosen.load()==0;
 std::string path=cf?(upload?"/__up":("/__down?bytes="+std::to_string(bytes))):
 (std::string(server().base)+(upload?"/empty.php":(bytes==0?"/empty.php":"/garbage.php?ckSize="+std::to_string((bytes+999)/1000))));
 std::string header=(upload?"POST ":"GET ")+path+
 " HTTP/1.1\r\nHost: "+std::string(server().host)+"\r\nUser-Agent: PS5WWWSpeedtest/0.3\r\nAccept-Encoding: identity\r\nConnection: close\r\n";
 if(upload)header+="Content-Type: application/octet-stream\r\nContent-Length: "+std::to_string(bytes)+"\r\n";
 header+="\r\n";
 auto began=Clock::now();
 if(!c.writeAll(header.data(),header.size())){error="HTTPS send failed";return false;}
 if(upload){
  char payload[16384];std::memset(payload,'x',sizeof(payload));
  for(size_t sent=0;sent<bytes;){size_t n=bytes-sent;if(n>sizeof(payload))n=sizeof(payload);
   if(!c.writeAll(payload,n)){error="Upload interrupted";return false;}sent+=n;
  }
 }
 char buffer[16384];std::string headers;bool parsed=false;int status=0;
 size_t received=0;bool complete=false;size_t contentLength=0;bool knownLength=false;
 while(true){
  int n=SSL_read(c.ssl,buffer,sizeof(buffer));
  if(n<=0)break;
  if(!parsed){
   headers.append(buffer,(size_t)n);
   if(headers.size()>32768){error="HTTP headers too large";return false;}
   auto pos=headers.find("\r\n\r\n");
   if(pos==std::string::npos)continue;
   if(std::sscanf(headers.c_str(),"HTTP/%*s %d",&status)!=1){error="Invalid HTTP response";return false;}
   std::string lower=headers.substr(0,pos);
   for(char& ch:lower)if(ch>='A'&&ch<='Z')ch=(char)(ch+32);
   auto p=lower.find("content-length:");
   if(p!=std::string::npos){contentLength=(size_t)std::strtoull(lower.c_str()+p+15,nullptr,10);knownLength=true;}
   if(lower.find("transfer-encoding: chunked")!=std::string::npos && !upload){error="Unexpected chunked test response";return false;}
   received=headers.size()-(pos+4);parsed=true;headers.clear();
  }else received+=(size_t)n;
  if(!upload&&received>bytes+4096){error="Response exceeded requested size";return false;}
  if(upload&&received>65536){error="Upload response too large";return false;}
  if(knownLength && received>=contentLength){complete=true;break;}
 }
 seconds=elapsed(began);
 if(!parsed){error="No HTTP response";return false;}
 if(status!=200){error="Test server HTTP "+std::to_string(status);return false;}
 if(!upload && cf && received!=bytes){error="Incomplete download";return false;}
 if(upload && knownLength && !complete){error="Incomplete upload response";return false;}
 if(seconds<=0){error="Invalid measurement duration";return false;}
 return true;
}
void* worker(void*) {
 std::string error;
 update(1,"Measuring latency...",.05);
 double last=0,total=0,jitterSum=0;int successes=0;
 for(int i=0;i<6;i++){
  double seconds=0;
  if(!request(false,0,seconds,error))break;
  double ms=seconds*1000;
  if(successes>0)jitterSum+=std::fabs(ms-last);
  last=ms;total+=ms;successes++;
  std::lock_guard<std::mutex> lock(guard);state.progress=.05+.15*(i+1)/6;
 }
 if(error.empty()&&successes>0){
  std::lock_guard<std::mutex> lock(guard);
  state.ping=total/successes;state.jitter=successes>1?jitterSum/(successes-1):0;
 }
 if(error.empty()){
  update(2,"Testing download...",.25);
  const size_t sizes[]={100000,1000000,4000000,8000000};
  double totalBytes=0,totalSec=0;
  for(int i=0;i<4;i++){
   double sec=0;if(!request(false,sizes[i],sec,error))break;
   totalBytes+=sizes[i];totalSec+=sec;
   std::lock_guard<std::mutex> lock(guard);
   state.down=totalBytes*8/totalSec/1e6;
   state.downGraph.push_back(sizes[i]*8/sec/1e6);state.liveMbps=sizes[i]*8/sec/1e6;
   state.progress=.25+.35*(i+1)/4;
  }
 }
 if(error.empty()){
  update(3,"Testing upload...",.65);
  const size_t sizes[]={100000,500000,1000000,2000000};
  double totalBytes=0,totalSec=0;
  for(int i=0;i<4;i++){
   double sec=0;if(!request(true,sizes[i],sec,error))break;
   totalBytes+=sizes[i];totalSec+=sec;
   std::lock_guard<std::mutex> lock(guard);
   state.up=totalBytes*8/totalSec/1e6;
   state.upGraph.push_back(sizes[i]*8/sec/1e6);state.liveMbps=sizes[i]*8/sec/1e6;
   state.progress=.65+.35*(i+1)/4;
  }
 }
 {
  std::lock_guard<std::mutex> lock(guard);
  state.running=false;state.completed=error.empty();state.phase=error.empty()?4:0;
  state.progress=error.empty()?1:state.progress;
  state.status=error.empty()?"Test complete":("Test failed: "+error);
 }
 busy.store(false,std::memory_order_release);
 return nullptr;
}
} // namespace
const Server* servers(){return serverList;}
int serverCount(){return count;}
int selectedServer(){return chosen.load();}
bool selectServer(int index){if(index<0||index>=count||busy.load())return false;chosen.store(index);return true;}
bool start(){
 bool expected=false;if(!busy.compare_exchange_strong(expected,true))return false;
 {std::lock_guard<std::mutex> lock(guard);state=Snapshot{};state.running=true;state.status=std::string("Connecting to ")+server().name+"...";}
 pthread_attr_t attrs;pthread_attr_init(&attrs);pthread_attr_setstacksize(&attrs,1024*1024);
 pthread_t id{};int result=pthread_create(&id,&attrs,worker,nullptr);pthread_attr_destroy(&attrs);
 if(result!=0){std::lock_guard<std::mutex> lock(guard);state.running=false;state.status="Could not launch network thread";busy=false;return false;}
 pthread_detach(id);return true;
}
Snapshot snapshot(){std::lock_guard<std::mutex> lock(guard);return state;}
}
