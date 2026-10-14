// SPDX-License-Identifier: GPL-3.0-or-later
// Compact BitTorrent v1 single-file downloader for PS5. SHA-1 piece verification;
// HTTP tracker + IPv4 TCP peers. The SHA-1 hash is required by the protocol.
#include "torrent_engine.hpp"
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "ca_bundle.inc"
#include <algorithm>
#include <atomic>
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
#include <unistd.h>
namespace torrent {
namespace {
const char* kHome="/data/ps5-torrent";
const char* kUsb="/mnt/usb0/ps5-torrent";
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
std::string chosenLog;
std::string ensureLogPath(){
 if(!chosenLog.empty())return chosenLog;
 // Prefer a log folder alongside the installed homebrew app.
 if(mkdirs("/data/homebrew/PPSA99684/logs"))chosenLog="/data/homebrew/PPSA99684/logs/torrent.log";
 else if(mkdirs("/data/ps5-torrent/logs"))chosenLog="/data/ps5-torrent/logs/torrent.log";
 else chosenLog="/data/ps5-torrent/torrent.log";
 return chosenLog;
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
 struct addrinfo hints{},*head=nullptr;hints.ai_family=AF_INET;hints.ai_socktype=SOCK_STREAM;
 char p[16];snprintf(p,sizeof(p),"%d",port);
 int code=getaddrinfo(host.c_str(),p,&hints,&head);
 if(code!=0){trace("DNS FAILED "+host+" getaddrinfo="+std::to_string(code));return -1;}
 int fd=-1;
 for(auto* a=head;a;a=a->ai_next){
  char address[INET_ADDRSTRLEN]={};
  if(a->ai_family==AF_INET)inet_ntop(AF_INET,&((struct sockaddr_in*)a->ai_addr)->sin_addr,address,sizeof(address));
  trace("Connecting TCP "+std::string(address)+":"+p);
  fd=socket(a->ai_family,a->ai_socktype,a->ai_protocol);
  if(fd<0){trace("Socket creation FAILED errno="+std::to_string(errno));continue;}
  int flags=fcntl(fd,F_GETFL,0);if(flags<0||fcntl(fd,F_SETFL,flags|O_NONBLOCK)!=0){trace("Nonblocking socket FAILED errno="+std::to_string(errno));close(fd);fd=-1;continue;}
  int ret=connect(fd,a->ai_addr,a->ai_addrlen);
  if(ret<0&&errno!=EINPROGRESS){trace("TCP connect FAILED errno="+std::to_string(errno));close(fd);fd=-1;continue;}
  fd_set w;FD_ZERO(&w);FD_SET(fd,&w);struct timeval tv{seconds,0};
  int selected=select(fd+1,nullptr,&w,nullptr,&tv);
  if(selected<=0){trace("TCP connect timeout/select FAILED errno="+std::to_string(errno));close(fd);fd=-1;continue;}
  int err=0;socklen_t z=sizeof(err);
  if(getsockopt(fd,SOL_SOCKET,SO_ERROR,&err,&z)!=0||err){trace("TCP connection refused/FAILED errno="+std::to_string(err?err:errno));close(fd);fd=-1;continue;}
  fcntl(fd,F_SETFL,flags);
  struct timeval t{seconds,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&t,sizeof(t));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&t,sizeof(t));
  trace("TCP connected "+std::string(address)+":"+p);
  break;
 }
 freeaddrinfo(head);
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
 const std::string req="GET "+q+" HTTP/1.1\r\nHost: "+u.host+"\r\nUser-Agent: TorrentFlow/0.1.2\r\nAccept: */*\r\nConnection: close\r\n\r\n";
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
bool grab(int fd,const Meta& m,uint32_t idx,std::string& bits,std::string& output,bool& choked,std::atomic<bool>& cancelled){
 const uint32_t n=pieceSize(m,idx);
 output.assign(n,0);
 const size_t blocks=(n+16383)/16384;
 std::vector<bool> got(blocks,false),requested(blocks,false);
 size_t finished=0;
 int outstanding=0,events=0;
 if(!sendMsg(fd,2)){trace("Peer: interested send FAILED");return false;}
 while(!cancelled.load()&&events++<256){
  if(!bits.empty()&&!hasPiece(bits,idx)){
   trace("Peer lacks piece "+std::to_string(idx)+"; switching to next available piece");
   return false;
  }
  if(!choked&&hasPiece(bits,idx)){
   for(size_t block=0;block<blocks&&outstanding<4;block++){
    if(requested[block]||got[block])continue;
    uint32_t offset=(uint32_t)block*16384;
    uint32_t len=std::min<uint32_t>(16384,n-offset);
    std::string req(12,0);store32(&req[0],idx);store32(&req[4],offset);store32(&req[8],len);
    if(!sendMsg(fd,6,req)){trace("Peer request send FAILED piece="+std::to_string(idx));return false;}
    requested[block]=true;outstanding++;
   }
  }
  char header[4];
  if(!recvAll(fd,header,4)){
   trace("Peer read timeout/disconnected piece="+std::to_string(idx)+" errno="+std::to_string(errno));
   return false;
  }
  uint32_t len=be32(header);
  if(len>1024*1024){trace("Peer frame exceeds size limit");return false;}
  if(len==0)continue;
  std::string msg(len,0);
  if(!recvAll(fd,&msg[0],len)){trace("Peer truncated frame piece="+std::to_string(idx));return false;}
  uint8_t id=uint8_t(msg[0]);
  if(id==0){choked=true;outstanding=0;for(size_t k=0;k<blocks;k++)if(!got[k])requested[k]=false;trace("Peer CHOKED");}
  else if(id==1){choked=false;trace("Peer UNCHOKED");}
  else if(id==5){bits=msg.substr(1);trace("Peer bitfield: "+std::to_string(bits.size())+" bytes");}
  else if(id==4&&len==5){uint32_t have=be32(msg.data()+1);if(have<(uint32_t)m.pieces.size()/20){if(have/8>=bits.size())bits.resize(have/8+1,0);bits[have/8]|=(char)(0x80>>(have%8));}}
  else if(id==7&&len>=9&&be32(msg.data()+1)==idx){
   uint32_t begin=be32(msg.data()+5),count=len-9;
   if(begin<n&&count<=n-begin&&begin%16384==0&&count==std::min<uint32_t>(16384,n-begin)){
    size_t b=begin/16384;
    if(!got[b]){
     memcpy(&output[begin],msg.data()+9,count);
     got[b]=true;finished++;
     if(outstanding>0)outstanding--;
    }
   }
   if(finished==blocks){
    if(shaPiece(m,idx,output))return true;
    trace("SHA-1 piece verification FAILED at "+std::to_string(idx));
    return false;
   }
  }
 }
 trace(cancelled.load()?"Piece transfer cancelled":"Peer frame limit exceeded / unresponsive");
 return false;
}
void saveState(const std::string& p,const std::vector<bool>& verified){std::string temp=p+".tmp";FILE* f=fopen(temp.c_str(),"wb");if(!f)return;for(bool b:verified)fputc(b?'1':'0',f);if(fclose(f)!=0){remove(temp.c_str());return;}rename(temp.c_str(),p.c_str());}
}
std::string logPath(){std::lock_guard<std::mutex> guard(logMutex);return ensureLogPath();}
std::vector<std::string> recentLogs(){std::lock_guard<std::mutex> guard(logMutex);return logRing;}
Engine::Engine(){state_.status="Ready";setUsb(false);trace("Torrent Flow v0.1.1 initialized - log file: "+logPath());}
Engine& engine(){static Engine instance;return instance;}
bool createDirectories(const std::string& path){return mkdirs(path)&&mkdirs(path+"/cache")&&mkdirs(path+"/downloads");}
const char* sampleTorrentName(){return kTest;}
const char* sampleTorrentPath(){return "/app0/assets/test/debian-13.7.0-amd64-netinst.iso.torrent";}
std::string testDisplayName(const std::string& path){
 for(const auto& test:kTests)if(path.size()>=strlen(test.file)&&path.compare(path.size()-strlen(test.file),strlen(test.file),test.file)==0)return test.display;
 return path;
}
std::string testSizeHint(const std::string& path){
 for(const auto& test:kTests)if(path.size()>=strlen(test.file)&&path.compare(path.size()-strlen(test.file),strlen(test.file),test.file)==0)return test.size;
 return "Unknown";
}
std::vector<std::string> discoverTorrents(){
 std::vector<std::string> out;
 const std::string roots[]={"/app0/assets/test","/data/homebrew/PPSA99684/assets/test","/data/ps5-torrent/cache","/mnt/usb0/ps5-torrent/cache"};
 for(const auto& t:kTests)for(const auto& root:roots){
  std::string path=root+"/"+t.file;
  struct stat st{};
  if(stat(path.c_str(),&st)==0&&S_ISREG(st.st_mode)&&st.st_size>0&&st.st_size<=1024*1024){
   out.push_back(path);break;
  }
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
 auto done=[&](const std::string& status,bool success){{std::lock_guard<std::mutex> g(mutex_);state_.running=false;state_.done=success;state_.status=status;state_.downBps=0;state_.activePeers=0;if(!success)state_.lastError=status;busy_=false;}trace(status);};
 auto cancelled=[&](){std::lock_guard<std::mutex> g(mutex_);return stopRequested_;};
 Meta m;std::string error;if(!metadata(path,m,error)){done(error,false);return;}trace("Metadata OK file="+m.name+" bytes="+std::to_string(m.bytes)+" pieces="+std::to_string(m.pieces.size()/20)+" tracker="+m.announce);
 std::string base;{std::lock_guard<std::mutex> g(mutex_);state_.name=m.name;state_.tracker=m.announce;state_.infoHash=hex((const unsigned char*)m.hash.data(),20);state_.totalBytes=m.bytes;state_.pieceCount=(int)m.pieces.size()/20;state_.verifiedPieces=0;state_.completedBytes=0;state_.loaded=true;base=state_.usb?kUsb:kHome;}
 if(!createDirectories(base)){done("Storage directory unavailable",false);return;}
 const std::string target=base+"/downloads/"+m.name,resume=base+"/cache/"+hex((const unsigned char*)m.hash.data(),20)+".resume";
 FILE* file=fopen(target.c_str(),"r+b");if(!file)file=fopen(target.c_str(),"w+b");if(!file){done("Cannot open destination file errno="+std::to_string(errno),false);return;}trace("Destination: "+target);
 std::vector<bool> verified(m.pieces.size()/20,false);
 // Never trust cached piece flags without verifying the actual data on disk.
 publish("Verifying existing pieces...");
 std::string data;for(size_t i=0;i<verified.size();++i){if(cancelled()){fclose(file);done("Stopped",false);return;}size_t n=pieceSize(m,(uint32_t)i);data.resize(n);if(fseek(file,(long)((uint64_t)i*m.piece),SEEK_SET)==0&&fread(&data[0],1,n,file)==n&&shaPiece(m,(uint32_t)i,data)){verified[i]=true;std::lock_guard<std::mutex> g(mutex_);state_.verifiedPieces++;state_.completedBytes+=n;}}
 if((size_t)std::count(verified.begin(),verified.end(),true)==verified.size()){fclose(file);done("Complete - verified all SHA-1 pieces",true);return;}
 // Initial release uses the supplied HTTP tracker and a limited peer selection.
 std::string peerId="-P5TO11-123456789012";
 int failedRounds=0;while(!cancelled()&&failedRounds<4){
  publish("Contacting tracker...");
  uint64_t left=0;for(size_t i=0;i<verified.size();i++)if(!verified[i])left+=pieceSize(m,(uint32_t)i);
  std::vector<Peer> peers;
  for(size_t t=0;t<m.trackers.size()&&!cancelled();t++){
   const std::string& announce=m.trackers[t];
   publish("Contacting tracker "+std::to_string(t+1)+"/"+std::to_string(m.trackers.size())+"...");
   if(announce.rfind("udp://",0)==0)peers=udpFallback(m,announce,peerId,left);
   else peers=tracker(m,announce,peerId,left);
   if(!peers.empty())break;
  }
  if(peers.empty()&&m.name=="debian-13.7.0-amd64-netinst.iso"&&m.announce.find("bttracker.debian.org")!=std::string::npos&&!cancelled()){
   publish("Debian tracker empty; trying UDP fallback...");
   peers=udpFallback(m,"udp://tracker.opentrackr.org:1337/announce",peerId,left);
  }
  {std::lock_guard<std::mutex> g(mutex_);state_.peerCount=(int)peers.size();}
  if(peers.empty()){failedRounds++;publish("Trackers returned no peers (see Logs)");std::this_thread::sleep_for(std::chrono::seconds(2));continue;}
  bool any=false;
  for(size_t peerIndex=0;peerIndex<peers.size();++peerIndex){
   const Peer& peer=peers[peerIndex];
   if(cancelled())break;
   publish("Connecting to peer "+std::to_string(peerIndex+1)+"/"+std::to_string(peers.size()));
   int fd=dial(peer.ip,peer.port,4);if(fd<0)continue;
   char handshake[68]={19};memcpy(handshake+1,"BitTorrent protocol",19);memcpy(handshake+28,m.hash.data(),20);memcpy(handshake+48,peerId.data(),20);
   char reply[68];bool ok=sendAll(fd,handshake,sizeof(handshake))&&recvAll(fd,reply,sizeof(reply))&&uint8_t(reply[0])==19&&memcmp(reply+1,"BitTorrent protocol",19)==0&&memcmp(reply+28,m.hash.data(),20)==0;
   if(!ok){trace("Peer handshake FAILED "+peer.ip+":"+std::to_string(peer.port));close(fd);continue;}trace("Peer handshake OK "+peer.ip+":"+std::to_string(peer.port));
   {std::lock_guard<std::mutex> g(mutex_);state_.activePeers=1;}
   std::string bitfield;bool peerChoked=true;publish("Downloading verified pieces...");
   // The peer loop is bounded by socket timeouts and server-provided bitfields.
   for(size_t i=0;i<verified.size()&&!cancelled();++i){
    if(verified[i])continue;
    if(i>0&&!hasPiece(bitfield,(uint32_t)i))continue;
    std::string piece;auto start=std::chrono::steady_clock::now();
    if(!grab(fd,m,(uint32_t)i,bitfield,piece,peerChoked,cancellation_)){
     if(!bitfield.empty()&&!hasPiece(bitfield,(uint32_t)i)&&!cancelled())continue;
     break;
    }
    if(fseek(file,(long)((uint64_t)i*m.piece),SEEK_SET)!=0||fwrite(piece.data(),1,piece.size(),file)!=piece.size()||fflush(file)!=0)break;
    any=true;verified[i]=true;saveState(resume,verified);trace("Piece "+std::to_string(i)+" verified and saved, bytes="+std::to_string(piece.size()));
    auto ms=std::max<int64_t>(1,std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count());
    {std::lock_guard<std::mutex> g(mutex_);state_.verifiedPieces++;state_.completedBytes+=piece.size();state_.transferredBytes+=piece.size();state_.downBps=piece.size()*1000.0/ms;}
   }
   close(fd);{std::lock_guard<std::mutex> g(mutex_);state_.activePeers=0;}
   if((size_t)std::count(verified.begin(),verified.end(),true)==verified.size())break;
  }
  if((size_t)std::count(verified.begin(),verified.end(),true)==verified.size())break;
  if(!any){failedRounds++;trace("No progress in tracker round "+std::to_string(failedRounds));}else failedRounds=0;
 }
 fclose(file);bool complete=std::all_of(verified.begin(),verified.end(),[](bool v){return v;});
 if(cancelled())done("Stopped - verified data preserved",false);
 else if(complete)done("Complete - verified all SHA-1 pieces",true);
 else done("No additional pieces available; retry with X",false);
}
}
