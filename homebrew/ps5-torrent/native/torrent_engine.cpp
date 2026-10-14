// SPDX-License-Identifier: GPL-3.0-or-later
// Compact BitTorrent v1 single-file downloader for PS5. SHA-1 piece verification;
// HTTP tracker + IPv4 TCP peers. The SHA-1 hash is required by the protocol.
#include "torrent_engine.hpp"
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "ca_bundle.inc"
#include <algorithm>
#include <atomic>
#include <random>
#include <chrono>
#include <cerrno>
#include <ctime>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
namespace torrent {
namespace {
#ifdef TORRENT_HOST_TEST
const char* kHome="/tmp/torrent-flow-host-test";
const char* kUsb="/tmp/torrent-flow-host-test-usb";
#else
const char* kHome="/data/ps5-torrent";
const char* kUsb="/mnt/usb0/ps5-torrent";
#endif
const char* kTest="debian-13.7.0-amd64-netinst.iso.torrent";
struct TestInfo {const char* file;const char* display;const char* size;};
const TestInfo kTests[]={
 {"debian-13.7.0-amd64-netinst.iso.torrent","Debian 13.7 netinst","~756 MiB"},
 {"ubuntu-26.04.1-live-server-amd64.iso.torrent","Ubuntu 26.04.1 LTS Server","~2.7 GiB"},
 {"ubuntu-24.04.5-live-server-amd64.iso.torrent","Ubuntu 24.04.5 LTS Server","~3.8 GiB"},
 {"linuxmint-22.3-cinnamon-64bit.iso.torrent","Linux Mint 22.3 Cinnamon","~2.9 GiB"},
 {"linuxmint-22.3-xfce-64bit.iso.torrent","Linux Mint 22.3 Xfce","~2.8 GiB"},
 {"linuxmint-22.3-mate-64bit.iso.torrent","Linux Mint 22.3 MATE","~2.9 GiB"}
};
bool isDir(const std::string& p){struct stat st{};return stat(p.c_str(),&st)==0&&S_ISDIR(st.st_mode);}
bool mkdirs(const std::string& p){if(p.empty()||p.size()>300||p[0]!='/')return false;std::string s;for(char c:p){s+=c;if(c=='/'&&s.size()>1)mkdir(s.c_str(),0755);}return mkdir(s.c_str(),0755)==0||isDir(s);}

std::mutex logMutex;
std::vector<std::string> logRing;
std::string ensureLogPath(){
#ifdef TORRENT_HOST_TEST
 mkdirs("/tmp/torrent-flow-host-test/logs");
 return "/tmp/torrent-flow-host-test/logs/torrent.log";
#else
 // Persist logs outside the install folder so updates never redirect them.
 mkdirs("/data/ps5-torrent/logs");
 return "/data/ps5-torrent/logs/torrent.log";
#endif
}
void trace(const std::string& message){
 std::lock_guard<std::mutex> guard(logMutex);
 const std::string path=ensureLogPath();
 struct stat st{};
 if(stat(path.c_str(),&st)==0&&st.st_size>1024*1024){
  remove((path+".old.log").c_str());
  rename(path.c_str(),(path+".old.log").c_str());
 }
 char header[60];snprintf(header,sizeof(header),"[%ld] ",(long)std::time(nullptr));
 std::string line=std::string(header)+message;
 FILE* file=fopen(path.c_str(),"ab");
 if(file){fprintf(file,"%s\n",line.c_str());fclose(file);}
 logRing.push_back(line);
 if(logRing.size()>70)logRing.erase(logRing.begin(),logRing.begin()+20);
}

bool readAll(const std::string& p,std::string& s){FILE* f=fopen(p.c_str(),"rb");if(!f)return false;if(fseek(f,0,SEEK_END)!=0){fclose(f);return false;}long n=ftell(f);if(n<=0||n>1024*1024){fclose(f);return false;}rewind(f);s.resize((size_t)n);bool ok=fread(&s[0],1,(size_t)n,f)==(size_t)n;fclose(f);return ok;}
struct B {
 std::string v;std::vector<B> list;std::map<std::string,B> dict;
 size_t start=0,end=0;char type='s';
 const B& get(const std::string& key)const{static B empty;auto i=dict.find(key);return i==dict.end()?empty:i->second;}
 int64_t number(int64_t fallback=0)const {if(type!='i')return fallback;char* e=nullptr;long long n=strtoll(v.c_str(),&e,10);return e&&!*e?n:fallback;}
};
bool parse(const std::string& s,size_t& pos,B& out,int depth=0){
 if(depth>24||pos>=s.size())return false;out.start=pos;
 if(s[pos]=='i'){out.type='i';size_t end=s.find('e',++pos);if(end==std::string::npos||end-pos>24||end==pos)return false;out.v=s.substr(pos,end-pos);pos=end+1;}
 else if(s[pos]=='l'||s[pos]=='d'){
  out.type=s[pos++];while(pos<s.size()&&s[pos]!='e'){if(out.list.size()+out.dict.size()>200000)return false;B item;if(out.type=='l'){if(!parse(s,pos,item,depth+1))return false;out.list.push_back(std::move(item));}else{B key;if(!parse(s,pos,key,depth+1)||key.type!='s'||key.v.size()>256||!parse(s,pos,item,depth+1))return false;out.dict[key.v]=std::move(item);}}
  if(pos>=s.size())return false;++pos;
 }else if(s[pos]>='0'&&s[pos]<='9'){
  size_t colon=s.find(':',pos);if(colon==std::string::npos||colon-pos>12)return false;unsigned long long len=0;for(size_t i=pos;i<colon;i++){if(s[i]<'0'||s[i]>'9')return false;len=len*10+(s[i]-'0');if(len>1024*1024)return false;}pos=colon+1;if(len>s.size()-pos)return false;out.v=s.substr(pos,(size_t)len);pos+=(size_t)len;
 }else return false;out.end=pos;return true;
}
std::string hex(const unsigned char* p,size_t n){static const char* digits="0123456789abcdef";std::string v;for(size_t i=0;i<n;i++){v+=digits[p[i]>>4];v+=digits[p[i]&15];}return v;}
std::string enc(const unsigned char* p,size_t n){static const char* digits="0123456789ABCDEF";std::string v;for(size_t i=0;i<n;i++){unsigned char c=p[i];if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~')v+=char(c);else{v+='%';v+=digits[c>>4];v+=digits[c&15];}}return v;}
struct Meta {std::string name,announce,hash,pieces;std::vector<std::string> trackers;uint64_t bytes=0;uint32_t piece=0;};
bool metadata(const std::string& path,Meta& m,std::string& error){
 std::string raw;if(!readAll(path,raw)){error="Cannot read .torrent metadata";return false;}
 B root;size_t pos=0;if(!parse(raw,pos,root)||pos!=raw.size()||root.type!='d'){error="Invalid bencoded torrent";return false;}
 const B& info=root.get("info");
 m.name=info.get("name").v;m.announce=root.get("announce").v;
 m.bytes=(uint64_t)std::max<int64_t>(0,info.get("length").number());
 m.piece=(uint32_t)std::max<int64_t>(0,info.get("piece length").number());
 m.pieces=info.get("pieces").v;
 if(info.type!='d'||info.get("files").type=='l'){error="Only single-file v1 torrents supported";return false;}
 if(m.name.empty()||m.name=="."||m.name==".."||m.name.find('/')!=std::string::npos||m.name.find('\\')!=std::string::npos||m.name.find("..")!=std::string::npos||m.name.find('\0')!=std::string::npos||m.name.size()>180){error="Unsafe torrent filename";return false;}
 if(!m.bytes||m.bytes>16ULL*1024*1024*1024||m.piece<16384||m.piece>4*1024*1024||m.pieces.size()%20||m.pieces.size()/20!=(m.bytes+m.piece-1)/m.piece){error="Invalid torrent size or piece hashes";return false;}
 auto addTracker=[&](const std::string& url){
  if((url.rfind("http://",0)==0||url.rfind("https://",0)==0||url.rfind("udp://",0)==0)&&url.size()<1000&&
     std::find(m.trackers.begin(),m.trackers.end(),url)==m.trackers.end()&&m.trackers.size()<12)m.trackers.push_back(url);
 };
 addTracker(m.announce);
 const B& tiers=root.get("announce-list");
 if(tiers.type=='l')for(const auto& tier:tiers.list){
  if(tier.type!='l')continue;
  for(const auto& value:tier.list)if(value.type=='s')addTracker(value.v);
 }
 if(m.trackers.empty()){error="No supported HTTP, HTTPS or UDP tracker in metadata";return false;}
 unsigned char digest[20];SHA1((const unsigned char*)raw.data()+info.start,info.end-info.start,digest);m.hash.assign((char*)digest,20);return true;
}
struct Url{std::string host,path;int port=80;bool tls=false;};
bool url(const std::string& input,Url& out){
 size_t prefix=0;
 if(input.rfind("https://",0)==0){prefix=8;out.tls=true;out.port=443;}
 else if(input.rfind("http://",0)==0){prefix=7;out.tls=false;out.port=80;}
 else return false;
 const std::string rem=input.substr(prefix);
 size_t slash=rem.find('/');const std::string authority=rem.substr(0,slash);
 out.path=slash==std::string::npos?"/":rem.substr(slash);
 size_t colon=authority.rfind(':');
 out.host=authority.substr(0,colon);
 if(colon!=std::string::npos){
  const std::string port=authority.substr(colon+1);
  if(port.empty()||port.size()>5||port.find_first_not_of("0123456789")!=std::string::npos)return false;
  out.port=atoi(port.c_str());
 }
 return !out.host.empty()&&out.host.size()<200&&out.port>0&&out.port<=65535&&out.path.size()<2000;
}
int dial(const std::string& host,int port,int seconds){
 trace("DNS lookup "+host+":"+std::to_string(port));
 struct addrinfo hints{},*head=nullptr;
 hints.ai_family=AF_INET;hints.ai_socktype=SOCK_STREAM;
 char portText[16];snprintf(portText,sizeof(portText),"%d",port);
 const int dns=getaddrinfo(host.c_str(),portText,&hints,&head);
 if(dns!=0){trace("DNS FAILED host="+host+" getaddrinfo="+std::to_string(dns));return -1;}
 int fd=-1;
 for(const addrinfo* a=head;a;a=a->ai_next){
  char address[INET_ADDRSTRLEN]={};
  if(a->ai_family==AF_INET){
   inet_ntop(AF_INET,&((const struct sockaddr_in*)a->ai_addr)->sin_addr,address,sizeof(address));
  }
  fd=socket(a->ai_family,a->ai_socktype,a->ai_protocol);
  if(fd<0){trace("TCP socket() FAILED errno="+std::to_string(errno));continue;}
  // PS5 homebrew sandbox: fcntl(F_SETFL,O_NONBLOCK) returned EACCES (errno=13)
  // for every peer. Use the blocking socket pattern already working in our
  // PS5 WWW Speedtest app. Install send/receive timeouts BEFORE connecting.
  struct timeval timeout{seconds,0};
  const int so=setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
  const int ro=setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
  if(so!=0||ro!=0){
   trace("TCP socket timeout options FAILED errno="+std::to_string(errno));
   close(fd);fd=-1;continue;
  }
  trace("TCP connecting (PS5 blocking socket) "+std::string(address)+":"+portText);
  const int ret=connect(fd,a->ai_addr,a->ai_addrlen);
  if(ret<0){
   const int error=errno;
   trace("TCP connect FAILED "+std::string(address)+":"+portText+" errno="+std::to_string(error));
   close(fd);fd=-1;continue;
  }
  trace("TCP CONNECTED "+std::string(address)+":"+portText);
  break;
 }
 freeaddrinfo(head);
 if(fd<0)trace("No TCP connection could be established for "+host+":"+portText);
 return fd;
}
bool sendAll(int fd,const void* data,size_t n){auto* p=(const char*)data;while(n){ssize_t k=send(fd,p,n,0);if(k<=0)return false;p+=k;n-=k;}return true;}
bool recvAll(int fd,void* data,size_t n){auto* p=(char*)data;while(n){ssize_t k=recv(fd,p,n,0);if(k<=0)return false;p+=k;n-=k;}return true;}
uint32_t be32(const char* p){return (uint32_t(uint8_t(p[0]))<<24)|(uint32_t(uint8_t(p[1]))<<16)|(uint32_t(uint8_t(p[2]))<<8)|uint8_t(p[3]);}
void store32(char* p,uint32_t n){for(int i=0;i<4;i++)p[i]=(char)(n>>(24-8*i));}
struct Peer{std::string ip;int port;};
bool httpResponse(const Url& u,const std::string& req,std::string& response){
 int fd=dial(u.host,u.port,8);if(fd<0){trace("Tracker connection FAILED");return false;}
 SSL_CTX* ctx=nullptr;SSL* ssl=nullptr;bool ok=false;
 if(u.tls){
  ctx=SSL_CTX_new(TLS_client_method());
  if(!ctx){trace("TLS context FAILED");close(fd);return false;}
  SSL_CTX_set_min_proto_version(ctx,TLS1_2_VERSION);
  SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,nullptr);
  BIO* input=BIO_new_mem_buf(kCaBundle,-1);
  int trusted=0;
  if(input){
   X509_STORE* store=SSL_CTX_get_cert_store(ctx);X509* cert=nullptr;
   while((cert=PEM_read_bio_X509(input,nullptr,nullptr,nullptr))!=nullptr){
    if(X509_STORE_add_cert(store,cert)==1)trusted++;
    X509_free(cert);
   }
   BIO_free(input);ERR_clear_error();
  }
  if(!trusted){trace("TLS trust store is empty");SSL_CTX_free(ctx);close(fd);return false;}
  ssl=SSL_new(ctx);
  if(!ssl){trace("TLS SSL_new FAILED");SSL_CTX_free(ctx);close(fd);return false;}
  SSL_set_fd(ssl,fd);SSL_set_tlsext_host_name(ssl,u.host.c_str());SSL_set1_host(ssl,u.host.c_str());
  if(SSL_connect(ssl)!=1||SSL_get_verify_result(ssl)!=X509_V_OK){
   trace("TLS handshake/certificate validation FAILED host="+u.host+" ssl_error="+std::to_string(ERR_get_error()));
  }else{
   size_t written=0;
   while(written<req.size()){
    int n=SSL_write(ssl,req.data()+written,(int)std::min<size_t>(16384,req.size()-written));
    if(n<=0){trace("TLS tracker send FAILED ssl_error="+std::to_string(SSL_get_error(ssl,n)));break;}
    written+=(size_t)n;
   }
   if(written==req.size()){
    char buff[4096];int n=0;
    while(response.size()<256*1024&&(n=SSL_read(ssl,buff,sizeof(buff)))>0)response.append(buff,(size_t)n);
    if(n<0)trace("TLS tracker receive error ssl_error="+std::to_string(SSL_get_error(ssl,n)));
    ok=!response.empty();
   }
  }
 }else{
  if(sendAll(fd,req.data(),req.size())){
   char buffer[4096];ssize_t got=0;
   while(response.size()<256*1024&&(got=recv(fd,buffer,sizeof(buffer),0))>0)response.append(buffer,(size_t)got);
   if(got<0)trace("HTTP tracker receive timeout errno="+std::to_string(errno));
   ok=!response.empty();
  }else trace("HTTP tracker send FAILED errno="+std::to_string(errno));
 }
 if(ssl)SSL_free(ssl);
 if(ctx)SSL_CTX_free(ctx);
 close(fd);
 return ok;
}
bool unchunk(std::string& body){
 std::string out;size_t pos=0;
 while(pos<body.size()){
  size_t end=body.find("\r\n",pos);if(end==std::string::npos||end-pos>32)return false;
  std::string hexLen=body.substr(pos,end-pos);size_t semicolon=hexLen.find(';');if(semicolon!=std::string::npos)hexLen.resize(semicolon);
  char* ep=nullptr;unsigned long n=strtoul(hexLen.c_str(),&ep,16);
  if(hexLen.empty()||!ep||*ep||n>256*1024)return false;
  pos=end+2;
  if(n==0){body.swap(out);return true;}
  if(n>body.size()-pos||pos+n+2>body.size()||body.compare(pos+n,2,"\r\n")!=0)return false;
  out.append(body.data()+pos,n);if(out.size()>256*1024)return false;
  pos+=n+2;
 }
 return false;
}
std::vector<Peer> tracker(const Meta& m,const std::string& announce,const std::string& peerId,uint64_t left){
 Url u;if(!url(announce,u)){trace("TRACKER invalid HTTP(S) announce URL");return {};}
 trace(std::string(u.tls?"HTTPS":"HTTP")+" tracker "+u.host+":"+std::to_string(u.port)+" announce...");
 std::string q=u.path+(u.path.find('?')==std::string::npos?"?":"&")+"info_hash="+enc((const unsigned char*)m.hash.data(),20)+"&peer_id="+enc((const unsigned char*)peerId.data(),20)+"&port=6889&uploaded=0&downloaded=0&left="+std::to_string(left)+"&compact=1&numwant=60&event=started";
 const std::string req="GET "+q+" HTTP/1.1\r\nHost: "+u.host+"\r\nUser-Agent: TorrentFlow/0.2.0\r\nAccept: */*\r\nConnection: close\r\n\r\n";
 std::string response;if(!httpResponse(u,req,response)){trace("Tracker response unavailable "+u.host);return {};}
 if(response.rfind("HTTP/1.",0)!=0||response.size()<12||response.substr(9,3)!="200"){
  trace("Tracker bad HTTP status: "+response.substr(0,std::min<size_t>(response.size(),120)));return {};
 }
 size_t body=response.find("\r\n\r\n");if(body==std::string::npos){trace("Tracker HTTP header parse FAILED");return {};}
 std::string headers=response.substr(0,body);
 body+=4;std::string data=response.substr(body);
 if(headers.find("Transfer-Encoding: chunked")!=std::string::npos||headers.find("transfer-encoding: chunked")!=std::string::npos){
  if(!unchunk(data)){trace("Tracker chunked HTTP payload malformed");return {};}
 }
 B root;size_t offset=0;
 if(!parse(data,offset,root)){trace("Tracker invalid bencode body="+std::to_string(data.size()));return {};}
 if(!root.get("failure reason").v.empty()){trace("Tracker rejected announce: "+root.get("failure reason").v);return {};}
 std::vector<Peer> peers;const B& a=root.get("peers");
 if(a.type=='s'){
  for(size_t i=0;i+6<=a.v.size()&&peers.size()<100;i+=6){
   char ip[INET_ADDRSTRLEN];if(!inet_ntop(AF_INET,a.v.data()+i,ip,sizeof(ip)))continue;
   int port=(uint8_t(a.v[i+4])<<8)|uint8_t(a.v[i+5]);if(port>0)peers.push_back({ip,port});
  }
 }else if(a.type=='l'){
  for(const auto& e:a.list){
   auto ip=e.get("ip").v;int port=(int)e.get("port").number();
   if(ip.size()<64&&port>0&&port<=65535&&peers.size()<100)peers.push_back({ip,port});
  }
 }
 trace("Tracker response OK: "+std::to_string(peers.size())+" peers");return peers;
}
void store64(char* p,uint64_t n){for(int i=0;i<8;i++)p[i]=(char)(n>>(56-8*i));}
std::vector<Peer> udpFallback(const Meta& m,const std::string& announce,const std::string& peerId,uint64_t left){
 if(announce.rfind("udp://",0)!=0){trace("Unsupported UDP tracker URL");return {};}
 const std::string authority=announce.substr(6,announce.find('/',6)==std::string::npos?std::string::npos:announce.find('/',6)-6);
 size_t colon=authority.rfind(':');
 if(colon==std::string::npos){trace("UDP tracker missing explicit port");return {};}
 const std::string host=authority.substr(0,colon),port=authority.substr(colon+1);
 if(host.empty()||host.size()>200||port.empty()||port.size()>5||port.find_first_not_of("0123456789")!=std::string::npos||atoi(port.c_str())==0||atoi(port.c_str())>65535){
  trace("UDP tracker invalid host or port");return {};
 }
 trace("UDP tracker "+host+":"+port);
 struct addrinfo hints{},*head=nullptr;hints.ai_family=AF_INET;hints.ai_socktype=SOCK_DGRAM;
 int result=getaddrinfo(host.c_str(),port.c_str(),&hints,&head);
 if(result!=0){trace("UDP tracker DNS FAILED "+std::to_string(result));return {};}
 int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0){freeaddrinfo(head);trace("UDP socket FAILED errno="+std::to_string(errno));return {};}
 struct timeval timeout{5,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
 if(connect(fd,head->ai_addr,head->ai_addrlen)!=0){trace("UDP tracker connect FAILED errno="+std::to_string(errno));freeaddrinfo(head);close(fd);return {};}
 freeaddrinfo(head);
 char req[98]={},resp[4096]={};
 store64(req,0x41727101980ULL);store32(req+8,0);store32(req+12,0x15104320);
 if(send(fd,req,16,0)!=16){trace("UDP tracker handshake send FAILED");close(fd);return {};}
 ssize_t n=recv(fd,resp,sizeof(resp),0);
 if(n<16||be32(resp)!=0||be32(resp+4)!=0x15104320){trace("UDP handshake timeout/rejected len="+std::to_string(n)+" errno="+std::to_string(errno));close(fd);return {};}
 memcpy(req,resp+8,8);store32(req+8,1);store32(req+12,0x15104321);
 memcpy(req+16,m.hash.data(),20);memcpy(req+36,peerId.data(),20);
 store64(req+56,0);store64(req+64,left);store64(req+72,0);store32(req+80,2);
 store32(req+84,0);store32(req+88,0x15381111);store32(req+92,0xffffffffU);req[96]=(char)(6889>>8);req[97]=(char)(6889&255);
 if(send(fd,req,sizeof(req),0)!=(ssize_t)sizeof(req)){trace("UDP announce send FAILED");close(fd);return {};}
 n=recv(fd,resp,sizeof(resp),0);close(fd);
 if(n<20||be32(resp)!=1||be32(resp+4)!=0x15104321){trace("UDP announce FAILED len="+std::to_string(n)+" errno="+std::to_string(errno));return {};}
 std::vector<Peer> peers;
 for(ssize_t i=20;i+6<=n&&peers.size()<100;i+=6){char ip[INET_ADDRSTRLEN];if(!inet_ntop(AF_INET,resp+i,ip,sizeof(ip)))continue;int port=(uint8_t(resp[i+4])<<8)|uint8_t(resp[i+5]);if(port>0)peers.push_back({ip,port});}
 trace("UDP tracker returned "+std::to_string(peers.size())+" peers");return peers;
}
bool sendMsg(int fd,uint8_t id,const std::string& body={}){std::string b(5+body.size(),0);store32(&b[0],(uint32_t)(body.size()+1));b[4]=(char)id;std::copy(body.begin(),body.end(),b.begin()+5);return sendAll(fd,b.data(),b.size());}
uint32_t pieceSize(const Meta& m,uint32_t index){uint64_t remain=m.bytes-(uint64_t)index*m.piece;return(uint32_t)std::min<uint64_t>(m.piece,remain);}
bool shaPiece(const Meta& m,uint32_t index,const std::string& bytes){unsigned char digest[20];SHA1((const unsigned char*)bytes.data(),bytes.size(),digest);return memcmp(digest,m.pieces.data()+index*20,20)==0;}
bool hasPiece(const std::string& bits,uint32_t index){return index/8<bits.size()&&(uint8_t(bits[index/8])&(0x80>>(index%8)));}
// Native BitTorrent wire session. A TCP handshake only identifies a peer;
// bitfield/HAVE and UNCHOKE must be processed before sending block requests.
struct Session{
 int fd=-1;
 bool choked=true, availability=false;
 std::string pieces;
 std::string address;
};
struct Frame{uint8_t id=255;std::string payload;};
bool readFrame(Session& s,Frame& frame){
 frame=Frame{};
 char hdr[4];
 if(!recvAll(s.fd,hdr,4)){trace("Peer read timeout/disconnect "+s.address+" errno="+std::to_string(errno));return false;}
 const uint32_t length=be32(hdr);
 if(length>1024*1024){trace("Peer sent oversized frame "+s.address+" bytes="+std::to_string(length));return false;}
 if(length==0)return true; // Keep-alive has no message ID.
 std::string buf(length,'\0');
 if(!recvAll(s.fd,&buf[0],length)){trace("Peer truncated frame "+s.address+" errno="+std::to_string(errno));return false;}
 frame.id=(uint8_t)buf[0];frame.payload.assign(buf.data()+1,length-1);
 return true;
}
bool updatePeer(Session& s,const Frame& f,uint32_t pieceCount){
 const size_t bytes=(pieceCount+7)/8;
 switch(f.id){
  case 0: s.choked=true;trace("Peer CHOKE "+s.address);return true;
  case 1: s.choked=false;trace("Peer UNCHOKE "+s.address);return true;
  case 4:
   if(f.payload.size()!=4){trace("Invalid HAVE "+s.address);return false;}
   {uint32_t idx=be32(f.payload.data());
    if(idx>=pieceCount){trace("Out of range HAVE "+s.address);return false;}
    if(s.pieces.empty())s.pieces.assign(bytes,'\0');
    s.pieces[idx/8]|=(char)(0x80>>(idx%8));s.availability=true;
   }
   return true;
  case 5:
   if(f.payload.size()!=bytes){trace("Invalid peer bitfield length "+s.address+" got="+std::to_string(f.payload.size())+" expected="+std::to_string(bytes));return false;}
   s.pieces=f.payload;s.availability=true;
   trace("Peer BITFIELD "+s.address+" bytes="+std::to_string(bytes));
   return true;
  case 14: // BEP-6 Fast Extension: HAVE_ALL.
   if(!f.payload.empty())return false;
   s.pieces.assign(bytes,(char)0xff);s.availability=true;
   trace("Peer HAVE_ALL "+s.address);return true;
  case 15: // BEP-6 Fast Extension: HAVE_NONE.
   if(!f.payload.empty())return false;
   s.pieces.assign(bytes,'\0');s.availability=true;
   trace("Peer HAVE_NONE "+s.address);return true;
  default: return true; // Standard/extension messages not relevant to a downloader.
 }
}
bool handshake(Session& s,const Meta& m,const std::string& peerId){
 char h[68]={19};memcpy(h+1,"BitTorrent protocol",19);memcpy(h+28,m.hash.data(),20);memcpy(h+48,peerId.data(),20);
 char response[68];
 if(!sendAll(s.fd,h,sizeof(h))||!recvAll(s.fd,response,sizeof(response))||uint8_t(response[0])!=19||
    memcmp(response+1,"BitTorrent protocol",19)!=0||memcmp(response+28,m.hash.data(),20)!=0){
  trace("Peer handshake rejected/timeout "+s.address);return false;
 }
 // Interested is sent once per connection, rather than once per candidate piece.
 if(!sendMsg(s.fd,2)){trace("Peer INTERESTED send failed "+s.address);return false;}
 trace("Peer handshake OK; interested sent "+s.address);
 return true;
}
bool awaitPeerReady(Session& s,const Meta& m,std::atomic<bool>& cancelled){
 const uint32_t count=(uint32_t)m.pieces.size()/20;
 for(int i=0;i<24&&!cancelled.load();i++){
  if(s.availability&&!s.choked)return true;
  Frame frame;
  if(!readFrame(s,frame))return false;
  if(!updatePeer(s,frame,count))return false;
 }
 trace("Peer not ready (unchoke/availability missing) "+s.address);
 return false;
}
bool downloadPiece(Session& s,const Meta& m,uint32_t idx,std::string& output,std::atomic<bool>& cancelled){
 const uint32_t n=pieceSize(m,idx),blockSize=16384;
 const size_t blockCount=(n+blockSize-1)/blockSize;
 if(blockCount==0||blockCount>256)return false;
 output.assign(n,'\0');
 std::vector<uint8_t> sent(blockCount,0),received(blockCount,0);
 size_t done=0,outstanding=0;
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(55);
 int frames=0;
 while(!cancelled.load()&&std::chrono::steady_clock::now()<deadline&&frames++<4096){
  if(s.availability&&!hasPiece(s.pieces,idx)){trace("Peer lost requested piece "+std::to_string(idx));return false;}
  if(!s.choked){
   for(size_t b=0;b<blockCount&&outstanding<8;b++){
    if(sent[b]||received[b])continue;
    const uint32_t offset=(uint32_t)b*blockSize,length=std::min(blockSize,n-offset);
    std::string request(12,'\0');
    store32(&request[0],idx);store32(&request[4],offset);store32(&request[8],length);
    if(!sendMsg(s.fd,6,request)){trace("Peer REQUEST send failed "+s.address);return false;}
    sent[b]=1;outstanding++;
   }
  }
  Frame frame;
  if(!readFrame(s,frame))return false;
  if(frame.id==7){
   if(frame.payload.size()<8){trace("Invalid PIECE frame "+s.address);return false;}
   uint32_t piece=be32(frame.payload.data()),offset=be32(frame.payload.data()+4);
   if(piece!=idx){trace("Unexpected PIECE index from "+s.address);continue;}
   uint32_t amount=(uint32_t)frame.payload.size()-8;
   if(offset>=n||offset%blockSize||amount!=std::min(blockSize,n-offset)){
    trace("Invalid PIECE block bounds from "+s.address);return false;
   }
   size_t b=offset/blockSize;
   if(!sent[b]){trace("Unsolicited PIECE block from "+s.address);return false;}
   if(!received[b]){
    memcpy(&output[offset],frame.payload.data()+8,amount);
    received[b]=1;done++;
    if(outstanding)outstanding--;
   }
   if(done==blockCount){
    if(!shaPiece(m,idx,output)){trace("SHA1 FAILED piece="+std::to_string(idx)+" peer="+s.address);return false;}
    return true;
   }
  }else{
   const bool wasChoked=s.choked;
   if(!updatePeer(s,frame,(uint32_t)m.pieces.size()/20))return false;
   if(!wasChoked&&s.choked){
    // A choke invalidates outstanding requests. Future UNCHOKE must re-request.
    outstanding=0;
    for(size_t b=0;b<blockCount;b++)if(!received[b])sent[b]=0;
   }
  }
 }
 trace("Peer piece deadline/timeout "+s.address+" piece="+std::to_string(idx));
 return false;
}
void saveState(const std::string& p,const std::vector<bool>& verified){std::string temp=p+".tmp";FILE* f=fopen(temp.c_str(),"wb");if(!f)return;for(bool b:verified)fputc(b?'1':'0',f);if(fclose(f)!=0){remove(temp.c_str());return;}rename(temp.c_str(),p.c_str());}
}
std::string logPath(){std::lock_guard<std::mutex> guard(logMutex);return ensureLogPath();}
std::vector<std::string> recentLogs(){std::lock_guard<std::mutex> guard(logMutex);return logRing;}
Engine::Engine(){state_.status="Ready";setUsb(false);trace("Torrent Flow v0.2.0 initialized - log file: "+logPath());}
Engine& engine(){static Engine instance;return instance;}
bool createDirectories(const std::string& path){return mkdirs(path)&&mkdirs(path+"/cache")&&mkdirs(path+"/downloads");}
const char* sampleTorrentName(){return kTest;}
const char* sampleTorrentPath(){return "/app0/assets/test/debian-13.7.0-amd64-netinst.iso.torrent";}
std::string testDisplayName(const std::string& path){
 for(const auto& test:kTests)if(path.size()>=strlen(test.file)&&path.compare(path.size()-strlen(test.file),strlen(test.file),test.file)==0)return test.display;
 const size_t slash=path.find_last_of('/');
 return slash==std::string::npos?path:path.substr(slash+1);
}
std::string testSizeHint(const std::string& path){
 for(const auto& test:kTests)if(path.size()>=strlen(test.file)&&path.compare(path.size()-strlen(test.file),strlen(test.file),test.file)==0)return test.size;
 return "Unknown";
}
std::vector<std::string> discoverTorrents(){
 std::vector<std::string> out;
 const std::string roots[]={"/app0/assets/test","/data/homebrew/PPSA99684/assets/test",
                            "/data/ps5-torrent/cache","/mnt/usb0/ps5-torrent/cache"};
 for(const auto& t:kTests)for(const auto& root:roots){
  std::string path=root+"/"+t.file;
  struct stat st{};
  if(stat(path.c_str(),&st)==0&&S_ISREG(st.st_mode)&&st.st_size>0&&st.st_size<=1024*1024){
   out.push_back(path);break;
  }
 }
 // In addition to the six bundled Linux tests, the library now discovers any
 // user-imported single-file .torrent placed in the cache directories via FTP/USB.
 for(const auto& root:{std::string("/data/ps5-torrent/cache"),std::string("/mnt/usb0/ps5-torrent/cache")}){
  DIR* dir=opendir(root.c_str());
  if(!dir)continue;
  struct dirent* entry=nullptr;
  while(out.size()<96&&(entry=readdir(dir))!=nullptr){
   const std::string name=entry->d_name;
   if(name.size()<=8||name.size()>200||name.find('/')!=std::string::npos||
      name=="."||name==".."||name.compare(name.size()-8,8,".torrent")!=0)continue;
   const std::string path=root+"/"+name;
   struct stat st{};
   if(stat(path.c_str(),&st)!=0||!S_ISREG(st.st_mode)||st.st_size<100||st.st_size>1024*1024)continue;
   if(std::find(out.begin(),out.end(),path)==out.end())out.push_back(path);
  }
  closedir(dir);
 }
 return out;
}
void Engine::setUsb(bool enable){std::lock_guard<std::mutex> lock(mutex_);if(busy_)return;state_.usb=enable;std::string base=enable?kUsb:kHome;state_.cacheDir=base+"/cache";state_.downloadDir=base+"/downloads";if(!enable)createDirectories(base);else if(!isDir("/mnt/usb0"))state_.status="USB 0 not mounted";else createDirectories(base);}
bool Engine::usingUsb()const{std::lock_guard<std::mutex> g(mutex_);return state_.usb;}
Snapshot Engine::snapshot()const{std::lock_guard<std::mutex> g(mutex_);return state_;}
void Engine::publish(const std::string& message){{std::lock_guard<std::mutex> g(mutex_);state_.status=message;}trace(message);}
void Engine::stop(){bool running=false;{std::lock_guard<std::mutex> g(mutex_);running=busy_;if(running){stopRequested_=true;cancellation_.store(true);state_.status="Stopping after current network operation...";}}if(running)trace("User requested transfer stop");}
bool Engine::start(const std::string& path){
 {std::lock_guard<std::mutex> lock(mutex_);if(busy_)return false;busy_=true;stopRequested_=false;state_.running=true;cancellation_.store(false);state_.paused=false;state_.done=false;state_.loaded=false;state_.lastError.clear();state_.downBps=0;state_.status="Reading torrent metadata...";}
 trace("START torrent: "+path);
 std::thread([this,path]{run(path);}).detach();return true;
}
void Engine::run(std::string path){
 using Clock=std::chrono::steady_clock;
 const auto cancelled=[&](){return cancellation_.load();};
 auto finish=[&](const std::string& status,bool success){
  {std::lock_guard<std::mutex> g(mutex_);
   state_.running=false;state_.done=success;state_.status=status;
   state_.downBps=0;state_.activePeers=0;
   if(!success)state_.lastError=status;
   busy_=false;
  }
  trace(status);
 };
 Meta m;std::string error;
 if(!metadata(path,m,error)){finish(error,false);return;}
 trace("Metadata OK file="+m.name+" bytes="+std::to_string(m.bytes)+
       " pieces="+std::to_string(m.pieces.size()/20)+" trackers="+std::to_string(m.trackers.size()));
 std::string base;
 {
  std::lock_guard<std::mutex> g(mutex_);
  state_.name=m.name;state_.tracker=m.announce;
  state_.infoHash=hex((const unsigned char*)m.hash.data(),20);
  state_.totalBytes=m.bytes;state_.pieceCount=(int)m.pieces.size()/20;
  state_.verifiedPieces=0;state_.completedBytes=0;state_.transferredBytes=0;
  state_.peerCount=0;state_.activePeers=0;state_.downBps=0;state_.loaded=true;
  base=state_.usb?kUsb:kHome;
 }
 if(!createDirectories(base)){finish("Storage directory unavailable",false);return;}
 const std::string target=base+"/downloads/"+m.name;
 const std::string resume=base+"/cache/"+hex((const unsigned char*)m.hash.data(),20)+".resume";
 FILE* initial=fopen(target.c_str(),"r+b");
 if(!initial)initial=fopen(target.c_str(),"w+b");
 if(!initial){finish("Cannot open download destination errno="+std::to_string(errno),false);return;}
 trace("Download destination: "+target);
 const size_t pieceCount=m.pieces.size()/20;
 std::vector<uint8_t> states(pieceCount,0); // 0=missing, 1=in flight, 2=verified
 std::vector<bool> verified(pieceCount,false);
 publish("Rechecking SHA-1 of stored pieces...");
 std::string check;
 for(size_t i=0;i<pieceCount&&!cancelled();i++){
  const uint32_t n=pieceSize(m,(uint32_t)i);
  check.resize(n);
  if(fseeko(initial,(off_t)((uint64_t)i*m.piece),SEEK_SET)==0 &&
     fread(&check[0],1,n,initial)==n &&
     shaPiece(m,(uint32_t)i,check)){
   states[i]=2;verified[i]=true;
   std::lock_guard<std::mutex> g(mutex_);
   state_.verifiedPieces++;state_.completedBytes+=n;
  }
  if((i%512)==511)trace("Resume verified "+std::to_string(i+1)+"/"+std::to_string(pieceCount)+" pieces scanned");
  clearerr(initial);
 }
 fclose(initial);
 if(cancelled()){finish("Stopped during resume verification",false);return;}
 if(std::all_of(states.begin(),states.end(),[](uint8_t n){return n==2;})){
  finish("Complete - all SHA-1 pieces verified",true);return;
 }
 unsigned char randomness[12]={};
 if(RAND_bytes(randomness,sizeof(randomness))!=1){
  const auto tick=(uint64_t)Clock::now().time_since_epoch().count();
  for(size_t i=0;i<sizeof(randomness);i++)randomness[i]=(unsigned char)(tick>>(i%8)*8);
 }
 std::string peerId="-TF0200-";
 for(unsigned char b:randomness)peerId+=(char)('A'+b%26);
 std::mutex stateGuard;
 std::mutex metricsGuard;
 std::atomic<int> completeCount{(int)std::count(states.begin(),states.end(),uint8_t(2))};
 std::atomic<bool> storageFailure{false};
 uint64_t sampleBytes=0;
 auto sampleTime=Clock::now();
 auto release=[&](uint32_t idx){
  std::lock_guard<std::mutex> g(stateGuard);
  if(states[idx]==1)states[idx]=0;
 };
 auto reserve=[&](const Session& session)->int{
  std::lock_guard<std::mutex> g(stateGuard);
  for(size_t i=0;i<states.size();i++){
   if(states[i]==0&&hasPiece(session.pieces,(uint32_t)i)){
    states[i]=1;return (int)i;
   }
  }
  return -1;
 };
 int emptyRounds=0,round=0;
 while(!cancelled()&&!storageFailure.load()&&completeCount.load()<(int)pieceCount&&emptyRounds<5){
  round++;
  publish("Discovering peers (round "+std::to_string(round)+")...");
  uint64_t remaining=0;
  for(size_t i=0;i<pieceCount;i++)if(states[i]!=2)remaining+=pieceSize(m,(uint32_t)i);
  std::vector<Peer> peers;
  for(size_t i=0;i<m.trackers.size()&&!cancelled();i++){
   const std::string& announce=m.trackers[i];
   publish("Tracker "+std::to_string(i+1)+"/"+std::to_string(m.trackers.size())+"...");
   auto found=(announce.rfind("udp://",0)==0)?
       udpFallback(m,announce,peerId,remaining):tracker(m,announce,peerId,remaining);
   peers.insert(peers.end(),found.begin(),found.end());
   if(peers.size()>=80)break;
  }
  if(peers.empty()&&m.name=="debian-13.7.0-amd64-netinst.iso"&&!cancelled()){
   auto fallback=udpFallback(m,"udp://tracker.opentrackr.org:1337/announce",peerId,remaining);
   peers.insert(peers.end(),fallback.begin(),fallback.end());
  }
  // Ignore duplicate tracker results and peers advertising invalid endpoints.
  std::sort(peers.begin(),peers.end(),[](const Peer& a,const Peer& b){
   return a.ip==b.ip?a.port<b.port:a.ip<b.ip;
  });
  peers.erase(std::unique(peers.begin(),peers.end(),[](const Peer& a,const Peer& b){
   return a.port==b.port&&a.ip==b.ip;
  }),peers.end());
  {std::lock_guard<std::mutex> g(mutex_);state_.peerCount=(int)peers.size();}
  trace("Discovery round "+std::to_string(round)+": "+std::to_string(peers.size())+" unique peers");
  if(peers.empty()){
   emptyRounds++;
   for(int k=0;k<20&&!cancelled();k++)std::this_thread::sleep_for(std::chrono::milliseconds(100));
   continue;
  }
  const int startPieces=completeCount.load();
  std::atomic<size_t> nextPeer{0};
  constexpr int maxWorkers=4;
  std::vector<std::thread> workers;
  workers.reserve(maxWorkers);
  for(int workerId=0;workerId<maxWorkers;workerId++){
   workers.emplace_back([&,workerId](){
    FILE* output=fopen(target.c_str(),"r+b");
    if(!output){
     storageFailure.store(true);trace("Worker file open FAILED errno="+std::to_string(errno));return;
    }
    while(!cancelled()&&!storageFailure.load()&&completeCount.load()<(int)pieceCount){
     size_t peerIndex=nextPeer.fetch_add(1);
     if(peerIndex>=peers.size())break;
     const Peer& p=peers[peerIndex];
     Session session;
     session.address=p.ip+":"+std::to_string(p.port);
     // Keep PS5-compatible BLOCKING sockets; fcntl(O_NONBLOCK) fails with EACCES.
     session.fd=dial(p.ip,p.port,7);
     if(session.fd<0)continue;
     if(!handshake(session,m,peerId)){close(session.fd);continue;}
     {std::lock_guard<std::mutex> g(mutex_);state_.activePeers++;}
     trace("Worker "+std::to_string(workerId)+" connected "+session.address);
     if(awaitPeerReady(session,m,cancellation_)){
      while(!cancelled()&&!storageFailure.load()&&completeCount.load()<(int)pieceCount){
       int idx=reserve(session);
       if(idx<0){
        trace("Peer has no further needed pieces "+session.address);
        break;
       }
       std::string piece;
       const auto begun=Clock::now();
       const bool received=downloadPiece(session,m,(uint32_t)idx,piece,cancellation_);
       if(!received){release((uint32_t)idx);break;}
       bool persisted=fseeko(output,(off_t)((uint64_t)idx*m.piece),SEEK_SET)==0 &&
           fwrite(piece.data(),1,piece.size(),output)==piece.size() && fflush(output)==0;
       if(!persisted){
        trace("STORAGE WRITE FAILURE piece="+std::to_string(idx)+" errno="+std::to_string(errno));
        release((uint32_t)idx);storageFailure.store(true);break;
       }
       {
        std::lock_guard<std::mutex> g(stateGuard);
        verified[(size_t)idx]=true;
        states[(size_t)idx]=2;  // Mark verified only AFTER persistent file write.
        saveState(resume,verified);
       }
       completeCount.fetch_add(1);
       {std::lock_guard<std::mutex> g(mutex_);
        state_.verifiedPieces++;state_.completedBytes+=piece.size();
        state_.transferredBytes+=piece.size();
        state_.status="Downloading: "+std::to_string(completeCount.load())+"/"+std::to_string(pieceCount)+" pieces";
       }
       {
        std::lock_guard<std::mutex> g(metricsGuard);
        sampleBytes+=piece.size();
        const auto now=Clock::now();
        double elapsed=std::chrono::duration<double>(now-sampleTime).count();
        if(elapsed>=1.0){
         const double instant=sampleBytes/elapsed;
         std::lock_guard<std::mutex> g2(mutex_);
         state_.downBps=state_.downBps>0?state_.downBps*0.6+instant*0.4:instant;
         sampleBytes=0;sampleTime=now;
        }
       }
       if((idx%16)==0||completeCount.load()==(int)pieceCount)
        trace("SHA1 verified piece="+std::to_string(idx)+" total="+std::to_string(completeCount.load())+"/"+std::to_string(pieceCount));
       (void)begun;
      }
     }else trace("Peer did not become ready "+session.address);
     close(session.fd);
     {std::lock_guard<std::mutex> g(mutex_);state_.activePeers--;}
    }
    fclose(output);
   });
  }
  for(auto& worker:workers)worker.join();
  if(completeCount.load()>startPieces){emptyRounds=0;trace("Round "+std::to_string(round)+" advanced "+std::to_string(completeCount.load()-startPieces)+" pieces");}
  else{emptyRounds++;trace("No verified pieces in round "+std::to_string(round));}
  if(!cancelled()&&completeCount.load()<(int)pieceCount){
   publish("Refreshing tracker; verified data retained");
   for(int i=0;i<10&&!cancelled();i++)std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
 }
 if(cancelled())finish("Stopped - verified pieces safely kept",false);
 else if(storageFailure.load())finish("Disk write error; see Logs",false);
 else if(completeCount.load()==(int)pieceCount)finish("Complete - all SHA-1 pieces verified",true);
 else finish("Stalled: no verified data from peers; check Logs and retry",false);
}
}
