// SPDX-License-Identifier: GPL-3.0-or-later
// Compact BitTorrent v1 single-file downloader for PS5. SHA-1 piece verification;
// HTTP tracker + IPv4 TCP peers. The SHA-1 hash is required by the protocol.
#include "torrent_engine.hpp"
#include <openssl/sha.h>
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
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
namespace torrent {
namespace {
const char* kHome="/data/ps5-torrent";
const char* kUsb="/mnt/usb0/ps5-torrent";
const char* kTest="debian-13.7.0-amd64-netinst.iso.torrent";
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
struct Meta {std::string name,announce,hash,pieces;uint64_t bytes=0;uint32_t piece=0;};
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
 if(m.announce.rfind("http://",0)!=0){error="Torrent needs an HTTP tracker (HTTPS/UDP unsupported in v0.1)";return false;}
 unsigned char digest[20];SHA1((const unsigned char*)raw.data()+info.start,info.end-info.start,digest);m.hash.assign((char*)digest,20);return true;
}
struct Url{std::string host,path;int port=80;};
bool url(const std::string& input,Url& out){
 if(input.rfind("http://",0)!=0)return false;std::string rem=input.substr(7);size_t slash=rem.find('/');std::string authority=rem.substr(0,slash);out.path=slash==std::string::npos?"/":rem.substr(slash);size_t colon=authority.find(':');out.host=authority.substr(0,colon);if(colon!=std::string::npos){std::string port=authority.substr(colon+1);if(port.empty()||port.size()>5||port.find_first_not_of("0123456789")!=std::string::npos)return false;out.port=atoi(port.c_str());}
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
  if(a->ai_family==AF_INET)inet_ntop(AF_INET,&((sockaddr_in*)a->ai_addr)->sin_addr,address,sizeof(address));
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
std::vector<Peer> tracker(const Meta& m,const std::string& peerId,uint64_t left){
 Url u;if(!url(m.announce,u)){trace("TRACKER invalid HTTP announce URL");return {};}
 trace("HTTP tracker "+u.host+":"+std::to_string(u.port)+" announce...");
 std::string q=u.path+(u.path.find('?')==std::string::npos?"?":"&")+"info_hash="+enc((const unsigned char*)m.hash.data(),20)+"&peer_id="+enc((const unsigned char*)peerId.data(),20)+"&port=6889&uploaded=0&downloaded=0&left="+std::to_string(left)+"&compact=1&numwant=60&event=started";
 int fd=dial(u.host,u.port,6);if(fd<0){trace("HTTP tracker connection FAILED");return {};}
 std::string req="GET "+q+" HTTP/1.0\r\nHost: "+u.host+"\r\nUser-Agent: TorrentFlow/0.1.1\r\nConnection: close\r\n\r\n";
 if(!sendAll(fd,req.data(),req.size())){trace("HTTP tracker send FAILED errno="+std::to_string(errno));close(fd);return {};}
 std::string response;char buffer[4096];ssize_t got=0;
 while(response.size()<256*1024&&(got=recv(fd,buffer,sizeof(buffer),0))>0)response.append(buffer,(size_t)got);
 if(got<0)trace("HTTP tracker read timeout/socket error errno="+std::to_string(errno));
 close(fd);
 if(response.rfind("HTTP/1.",0)!=0||response.size()<12||response.substr(9,3)!="200"){
  trace("HTTP tracker bad status: "+response.substr(0,std::min<size_t>(response.size(),80)));return {};
 }
 size_t body=response.find("\r\n\r\n");if(body==std::string::npos){trace("HTTP tracker header parse FAILED");return {};}body+=4;
 B root;size_t offset=0;std::string data=response.substr(body);
 if(!parse(data,offset,root)){trace("HTTP tracker bencode invalid; bytes="+std::to_string(data.size()));return {};}
 if(!root.get("failure reason").v.empty()){trace("HTTP tracker rejected: "+root.get("failure reason").v);return {};}
 std::vector<Peer> peers;const B& a=root.get("peers");
 if(a.type=='s'){for(size_t i=0;i+6<=a.v.size()&&peers.size()<100;i+=6){char ip[INET_ADDRSTRLEN];if(!inet_ntop(AF_INET,a.v.data()+i,ip,sizeof(ip)))continue;int port=(uint8_t(a.v[i+4])<<8)|uint8_t(a.v[i+5]);if(port>0)peers.push_back({ip,port});}}
 else if(a.type=='l'){for(const auto& e:a.list){auto ip=e.get("ip").v;int port=(int)e.get("port").number();if(ip.size()<64&&port>0&&port<=65535&&peers.size()<100)peers.push_back({ip,port});}}
 trace("HTTP tracker returned "+std::to_string(peers.size())+" peers");return peers;
}
void store64(char* p,uint64_t n){for(int i=0;i<8;i++)p[i]=(char)(n>>(56-8*i));}
std::vector<Peer> udpFallback(const Meta& m,const std::string& peerId,uint64_t left){
 const std::string host="tracker.opentrackr.org";
 trace("Debian fallback: UDP tracker "+host+":1337");
 struct addrinfo hints{},*head=nullptr;hints.ai_family=AF_INET;hints.ai_socktype=SOCK_DGRAM;
 int result=getaddrinfo(host.c_str(),"1337",&hints,&head);
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
bool grab(int fd,const Meta& m,uint32_t idx,std::string& bits,std::string& output,std::atomic<bool>& cancelled){
 const uint32_t n=pieceSize(m,idx);output.assign(n,0);std::vector<bool> got((n+16383)/16384,false);
 int requested=0,finished=0,idle=0;bool interested=false;
 while(!cancelled&&idle++<120){
  if(!interested){if(!sendMsg(fd,2))return false;interested=true;}
  if(hasPiece(bits,idx)&&requested==0){for(uint32_t offset=0;offset<n;offset+=16384){uint32_t len=std::min<uint32_t>(16384,n-offset);std::string req(12,0);store32(&req[0],idx);store32(&req[4],offset);store32(&req[8],len);if(!sendMsg(fd,6,req))return false;requested++;}}
  uint32_t len=0;char head[4];if(!recvAll(fd,head,4))return false;len=be32(head);if(len>1024*1024)return false;if(!len)continue;
  std::string msg(len,0);if(!recvAll(fd,&msg[0],len))return false;uint8_t id=uint8_t(msg[0]);
  if(id==5){bits=msg.substr(1);idle=0;}
  if(id==4&&len==5){uint32_t have=be32(msg.data()+1);if(have/8>=bits.size())bits.resize(have/8+1,0);bits[have/8]|=(char)(0x80>>(have%8));idle=0;}
  if(id==7&&len>=9&&be32(msg.data()+1)==idx){uint32_t begin=be32(msg.data()+5);uint32_t count=len-9;if(begin<n&&count<=n-begin&&begin%16384==0&&count<=16384){memcpy(&output[begin],msg.data()+9,count);int block=begin/16384;if(!got[block]){got[block]=true;finished++;}idle=0;}}
  if(requested&&finished==requested)return shaPiece(m,idx,output);
 }
 return false;
}
void saveState(const std::string& p,const std::vector<bool>& verified){std::string temp=p+".tmp";FILE* f=fopen(temp.c_str(),"wb");if(!f)return;for(bool b:verified)fputc(b?'1':'0',f);if(fclose(f)!=0){remove(temp.c_str());return;}rename(temp.c_str(),p.c_str());}
}
Engine::Engine(){state_.status="Ready";setUsb(false);}
Engine& engine(){static Engine instance;return instance;}
bool createDirectories(const std::string& path){return mkdirs(path)&&mkdirs(path+"/cache")&&mkdirs(path+"/downloads");}
const char* sampleTorrentName(){return kTest;}
const char* sampleTorrentPath(){return "/app0/assets/test/debian-13.7.0-amd64-netinst.iso.torrent";}
std::vector<std::string> discoverTorrents(){
 std::vector<std::string> out;const std::string roots[]={"/app0/assets/test","/data/ps5-torrent/cache","/mnt/usb0/ps5-torrent/cache"};
 for(const auto& p:roots){std::string path=p+"/"+kTest;struct stat st{};if(stat(path.c_str(),&st)==0&&S_ISREG(st.st_mode))out.push_back(path);}
 return out;
}
void Engine::setUsb(bool enable){std::lock_guard<std::mutex> lock(mutex_);if(busy_)return;state_.usb=enable;std::string base=enable?kUsb:kHome;state_.cacheDir=base+"/cache";state_.downloadDir=base+"/downloads";if(!enable)createDirectories(base);else if(!isDir("/mnt/usb0"))state_.status="USB 0 not mounted";else createDirectories(base);}
bool Engine::usingUsb()const{std::lock_guard<std::mutex> g(mutex_);return state_.usb;}
Snapshot Engine::snapshot()const{std::lock_guard<std::mutex> g(mutex_);return state_;}
void Engine::publish(const std::string& message){std::lock_guard<std::mutex> g(mutex_);state_.status=message;}
void Engine::stop(){std::lock_guard<std::mutex> g(mutex_);stopRequested_=true;state_.status="Stopping after current network operation...";}
bool Engine::start(const std::string& path){
 {std::lock_guard<std::mutex> lock(mutex_);if(busy_)return false;busy_=true;stopRequested_=false;state_.running=true;state_.paused=false;state_.done=false;state_.loaded=false;state_.lastError.clear();state_.downBps=0;state_.status="Reading torrent metadata...";}
 std::thread([this,path]{run(path);}).detach();return true;
}
void Engine::run(std::string path){
 auto done=[&](const std::string& status,bool success){std::lock_guard<std::mutex> g(mutex_);state_.running=false;state_.done=success;state_.status=status;busy_=false;};
 auto cancelled=[&](){std::lock_guard<std::mutex> g(mutex_);return stopRequested_;};
 Meta m;std::string error;if(!metadata(path,m,error)){std::lock_guard<std::mutex> g(mutex_);state_.lastError=error;state_.status=error;state_.running=false;busy_=false;return;}
 std::string base;{std::lock_guard<std::mutex> g(mutex_);state_.name=m.name;state_.tracker=m.announce;state_.infoHash=hex((const unsigned char*)m.hash.data(),20);state_.totalBytes=m.bytes;state_.pieceCount=(int)m.pieces.size()/20;state_.verifiedPieces=0;state_.completedBytes=0;state_.loaded=true;base=state_.usb?kUsb:kHome;}
 if(!createDirectories(base)){done("Storage directory unavailable",false);return;}
 const std::string target=base+"/downloads/"+m.name,resume=base+"/cache/"+hex((const unsigned char*)m.hash.data(),20)+".resume";
 FILE* file=fopen(target.c_str(),"r+b");if(!file)file=fopen(target.c_str(),"w+b");if(!file){done("Cannot open destination file",false);return;}
 std::vector<bool> verified(m.pieces.size()/20,false);
 // Never trust cached piece flags without verifying the actual data on disk.
 publish("Verifying existing pieces...");
 std::string data;for(size_t i=0;i<verified.size();++i){if(cancelled()){fclose(file);done("Stopped",false);return;}size_t n=pieceSize(m,(uint32_t)i);data.resize(n);if(fseek(file,(long)((uint64_t)i*m.piece),SEEK_SET)==0&&fread(&data[0],1,n,file)==n&&shaPiece(m,(uint32_t)i,data)){verified[i]=true;std::lock_guard<std::mutex> g(mutex_);state_.verifiedPieces++;state_.completedBytes+=n;}}
 if((size_t)std::count(verified.begin(),verified.end(),true)==verified.size()){fclose(file);done("Complete - verified all SHA-1 pieces",true);return;}
 // Initial release uses the supplied HTTP tracker and a limited peer selection.
 std::string peerId="-P5TO01-123456789012";std::atomic<bool> dummy{false};
 int failedRounds=0;while(!cancelled()&&failedRounds<4){
  publish("Contacting tracker...");
  uint64_t left=0;for(size_t i=0;i<verified.size();i++)if(!verified[i])left+=pieceSize(m,(uint32_t)i);
  auto peers=tracker(m,peerId,left);{std::lock_guard<std::mutex> g(mutex_);state_.peerCount=(int)peers.size();}
  if(peers.empty()){failedRounds++;publish("Tracker returned no reachable peers");std::this_thread::sleep_for(std::chrono::seconds(2));continue;}
  bool any=false;
  for(const Peer& peer:peers){
   if(cancelled())break;
   int fd=dial(peer.ip,peer.port,4);if(fd<0)continue;
   char handshake[68]={19};memcpy(handshake+1,"BitTorrent protocol",19);memcpy(handshake+28,m.hash.data(),20);memcpy(handshake+48,peerId.data(),20);
   char reply[68];bool ok=sendAll(fd,handshake,sizeof(handshake))&&recvAll(fd,reply,sizeof(reply))&&uint8_t(reply[0])==19&&memcmp(reply+1,"BitTorrent protocol",19)==0&&memcmp(reply+28,m.hash.data(),20)==0;
   if(!ok){close(fd);continue;}
   {std::lock_guard<std::mutex> g(mutex_);state_.activePeers=1;}
   std::string bitfield;publish("Downloading verified pieces...");
   // The peer loop is bounded by socket timeouts and server-provided bitfields.
   for(size_t i=0;i<verified.size()&&!cancelled();++i){
    if(verified[i])continue;
    if(i>0&&!hasPiece(bitfield,(uint32_t)i))continue;
    std::string piece;auto start=std::chrono::steady_clock::now();
    if(!grab(fd,m,(uint32_t)i,bitfield,piece,dummy))break;
    if(fseek(file,(long)((uint64_t)i*m.piece),SEEK_SET)!=0||fwrite(piece.data(),1,piece.size(),file)!=piece.size()||fflush(file)!=0)break;
    any=true;verified[i]=true;saveState(resume,verified);
    auto ms=std::max<int64_t>(1,std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count());
    {std::lock_guard<std::mutex> g(mutex_);state_.verifiedPieces++;state_.completedBytes+=piece.size();state_.transferredBytes+=piece.size();state_.downBps=piece.size()*1000.0/ms;}
   }
   close(fd);{std::lock_guard<std::mutex> g(mutex_);state_.activePeers=0;}
   if((size_t)std::count(verified.begin(),verified.end(),true)==verified.size())break;
  }
  if((size_t)std::count(verified.begin(),verified.end(),true)==verified.size())break;
  if(!any)failedRounds++;else failedRounds=0;
 }
 fclose(file);bool complete=std::all_of(verified.begin(),verified.end(),[](bool v){return v;});
 if(cancelled())done("Stopped - verified data preserved",false);
 else if(complete)done("Complete - verified all SHA-1 pieces",true);
 else done("No additional pieces available; retry with X",false);
}
}
