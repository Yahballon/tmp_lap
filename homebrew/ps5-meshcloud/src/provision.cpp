// SPDX-License-Identifier: GPL-3.0-or-later
#include "provision.hpp"
#include "core.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <memory>
#include <system_error>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace meshcloud::provision {
namespace {
namespace fs=std::filesystem;
#ifdef _WIN32
using Socket=SOCKET;
constexpr Socket invalid=INVALID_SOCKET;
void sock_close(Socket s){if(s!=invalid)closesocket(s);}
bool sockets_ready(){
 static const bool value=[](){WSADATA a{};return WSAStartup(MAKEWORD(2,2),&a)==0;}();
 return value;
}
#else
using Socket=int;
constexpr Socket invalid=-1;
void sock_close(Socket s){if(s>=0)close(s);}
bool sockets_ready(){return true;}
#endif
struct SocketOwner{
 Socket socket=invalid;
 explicit SocketOwner(Socket s=invalid):socket(s){}
 ~SocketOwner(){sock_close(socket);}
 SocketOwner(const SocketOwner&)=delete;
 SocketOwner& operator=(const SocketOwner&)=delete;
};
using TLSContext=std::unique_ptr<SSL_CTX,decltype(&SSL_CTX_free)>;
using TLSSession=std::unique_ptr<SSL,decltype(&SSL_free)>;
using Certificate=std::unique_ptr<X509,decltype(&X509_free)>;
using Public=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
constexpr std::size_t max_ledger_packet=8*1024*1024;
constexpr unsigned char ready[4]={'M','C','R','D'};
constexpr unsigned char success[4]={'M','C','O','K'};
bool get_public(EVP_PKEY* key,admission::PublicKey& out){
 out={};std::size_t len=out.size();
 return key && EVP_PKEY_base_id(key)==EVP_PKEY_ED25519 &&
  EVP_PKEY_get_raw_public_key(key,out.data(),&len)==1&&len==out.size();
}
bool known(SSL* ssl,const std::vector<admission::PublicKey>& pinned){
 Certificate cert(SSL_get1_peer_certificate(ssl),X509_free);
 if(!cert||pinned.empty())return false;
 Public pub(X509_get_pubkey(cert.get()),EVP_PKEY_free);
 admission::PublicKey actual{};
 return pub&&get_public(pub.get(),actual)&&
  std::find(pinned.begin(),pinned.end(),actual)!=pinned.end();
}
TLSContext tls_context(EVP_PKEY* key,bool listener){
 TLSContext ctx(SSL_CTX_new(TLS_method()),SSL_CTX_free);
 if(!ctx||!key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519||
  SSL_CTX_set_min_proto_version(ctx.get(),TLS1_3_VERSION)!=1||
  SSL_CTX_set_max_proto_version(ctx.get(),TLS1_3_VERSION)!=1)
  return {nullptr,SSL_CTX_free};
 SSL_CTX_set_verify(ctx.get(),SSL_VERIFY_PEER|
   (listener?SSL_VERIFY_FAIL_IF_NO_PEER_CERT:0),[](int,X509_STORE_CTX*){return 1;});
 Certificate cert(X509_new(),X509_free);
 if(!cert||X509_set_version(cert.get(),2)!=1||
    ASN1_INTEGER_set(X509_get_serialNumber(cert.get()),1)!=1||
    !X509_gmtime_adj(X509_getm_notBefore(cert.get()),-60)||
    !X509_gmtime_adj(X509_getm_notAfter(cert.get()),24*3600)||
    X509_set_pubkey(cert.get(),key)!=1)return {nullptr,SSL_CTX_free};
 X509_NAME* dn=X509_get_subject_name(cert.get());
 if(!dn||X509_NAME_add_entry_by_txt(dn,"CN",MBSTRING_ASC,
    reinterpret_cast<const unsigned char*>("MeshCloud 13GiB Provisioning"),
    -1,-1,0)!=1||X509_set_issuer_name(cert.get(),dn)!=1||
    X509_sign(cert.get(),key,nullptr)<=0||
    SSL_CTX_use_certificate(ctx.get(),cert.get())!=1||
    SSL_CTX_use_PrivateKey(ctx.get(),key)!=1||
    SSL_CTX_check_private_key(ctx.get())!=1)return {nullptr,SSL_CTX_free};
 SSL_CTX_set_options(ctx.get(),SSL_OP_NO_COMPRESSION);
 return ctx;
}
bool set_timeout(Socket fd){
#ifdef _WIN32
 DWORD ms=45000;
 return setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,
    reinterpret_cast<const char*>(&ms),sizeof(ms))==0 &&
  setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,
    reinterpret_cast<const char*>(&ms),sizeof(ms))==0;
#else
 timeval t{};t.tv_sec=45;
 return setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&t,sizeof(t))==0 &&
  setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&t,sizeof(t))==0;
#endif
}
bool address_of(const std::string& ip,std::uint16_t number,sockaddr_in& target,bool listen_mode){
 target={};target.sin_family=AF_INET;target.sin_port=htons(number);
 if(number==0||inet_pton(AF_INET,ip.c_str(),&target.sin_addr)!=1)return false;
 return !listen_mode || target.sin_addr.s_addr!=INADDR_ANY;
}
bool read_exact(SSL* ssl,unsigned char* dest,std::size_t size) {
 std::size_t at=0;
 while(at<size){
  int n=SSL_read(ssl,dest+at,static_cast<int>(std::min<std::size_t>(size-at,16384)));
  if(n<=0)return false;
  at+=static_cast<std::size_t>(n);
 }
 return true;
}
bool write_exact(SSL* ssl,const unsigned char* source,std::size_t size) {
 std::size_t at=0;
 while(at<size){
  int n=SSL_write(ssl,source+at,static_cast<int>(std::min<std::size_t>(size-at,16384)));
  if(n<=0)return false;
  at+=static_cast<std::size_t>(n);
 }
 return true;
}
bool send_small(SSL* ssl,const std::vector<unsigned char>& data){
 if(data.empty()||data.size()>max_ledger_packet)return false;
 std::uint32_t len=static_cast<std::uint32_t>(data.size());
 unsigned char hdr[4]{static_cast<unsigned char>(len>>24),
  static_cast<unsigned char>(len>>16),static_cast<unsigned char>(len>>8),
  static_cast<unsigned char>(len)};
 return write_exact(ssl,hdr,4)&&write_exact(ssl,data.data(),data.size());
}
bool recv_small(SSL* ssl,std::vector<unsigned char>& data){
 data.clear();unsigned char hdr[4]{};
 if(!read_exact(ssl,hdr,4))return false;
 const auto len=(static_cast<std::uint32_t>(hdr[0])<<24)|
                (static_cast<std::uint32_t>(hdr[1])<<16)|
                (static_cast<std::uint32_t>(hdr[2])<<8)|hdr[3];
 if(len==0||len>max_ledger_packet)return false;
 data.resize(len);
 return read_exact(ssl,data.data(),len);
}
bool transfer_tls(SSL* session,Socket fd,bool server) {
 return set_timeout(fd)&&SSL_set_fd(session,static_cast<int>(fd))==1 &&
   (server?SSL_accept(session):SSL_connect(session))==1;
}
bool is_absent(const fs::path& file){
 std::error_code ec;
 const auto status=fs::symlink_status(file,ec);
 return status.type()==fs::file_type::not_found &&
  (!ec||ec==std::errc::no_such_file_or_directory);
}
int exclusive(const fs::path& file) {
#ifdef _WIN32
 int fd=-1;
 if(_sopen_s(&fd,file.string().c_str(),_O_CREAT|_O_EXCL|_O_BINARY|_O_WRONLY,
             _SH_DENYRW,_S_IREAD|_S_IWRITE)!=0)return -1;
 return fd;
#else
 return open(file.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
#endif
}
bool fd_write(int fd,const unsigned char* p,std::size_t size){
 std::size_t offset=0;
 while(offset<size){
#ifdef _WIN32
  const auto n=_write(fd,p+offset,static_cast<unsigned int>(size-offset));
#else
  const auto n=write(fd,p+offset,size-offset);
#endif
  if(n<=0)return false;
  offset+=static_cast<std::size_t>(n);
 }
 return true;
}
bool fd_close(int fd) {
#ifdef _WIN32
 const auto okay=_commit(fd)==0;
 return _close(fd)==0&&okay;
#else
 const auto okay=fsync(fd)==0;
 return close(fd)==0&&okay;
#endif
}
bool hashes(const unsigned char* bytes,std::size_t length,
            const storage_audit::Digest& expected){
 storage_audit::Digest hash{};
 unsigned n=0;
 return EVP_Digest(bytes,length,hash.data(),&n,EVP_sha256(),nullptr)==1 &&
        n==hash.size()&&hash==expected;
}
bool ingest(SSL* tls,const fs::path& data_path,const fs::path& ledger_path,
  const admission::PublicKey& identity,std::uint64_t exact_bytes,std::string& error,
  const Progress& progress){
 if(!is_absent(data_path)||!is_absent(ledger_path)){
  error="Audit sentinel or ledger already exists; refusing overwrite";return false;
 }
 const auto partial_data=fs::path(data_path.string()+".pending");
 const auto partial_ledger=fs::path(ledger_path.string()+".pending");
 if(!is_absent(partial_data)||!is_absent(partial_ledger)){
  error="Unfinished provisioning staging files exist; manually review/clean";return false;
 }
 std::vector<unsigned char> encoded;
 if(!recv_small(tls,encoded)){
  error="Missing or oversized authenticated SHA256 ledger";return false;
 }
 const int ledger_fd=exclusive(partial_ledger);
 if(ledger_fd<0){error="Cannot exclusively create staging ledger";return false;}
 const bool saved=fd_write(ledger_fd,encoded.data(),encoded.size());
 const bool flushed=fd_close(ledger_fd);
 if(!saved||!flushed){
  std::error_code ec;fs::remove(partial_ledger,ec);
  error="Could not durably stage auditor hash ledger";return false;
 }
 storage_audit::Ledger ledger{};
 const bool eligible=storage_audit::load_ledger(partial_ledger,ledger) &&
      ledger.bytes==exact_bytes && ledger.node==identity;
 if(!eligible){
  std::error_code ec;fs::remove(partial_ledger,ec);
  error="Wrong node, capacity or corrupt independent ledger";return false;
 }
 std::error_code ec;
 const auto parent=data_path.parent_path().empty()?fs::path("."):data_path.parent_path();
 const auto space=fs::space(parent,ec);
 if(ec || (exact_bytes==meshcloud::minimum_bytes &&
   (space.available<exact_bytes||
    space.available-exact_bytes<meshcloud::safety_bytes))){
  fs::remove(partial_ledger,ec);
  error="13 GiB storage plus 2 GiB margin not available";return false;
 }
 // Only after validating the pinned auditor and its ledger do we allow bytes.
 if(!write_exact(tls,ready,sizeof(ready))){
  fs::remove(partial_ledger,ec);error="Could not acknowledge authenticated ledger";return false;
 }
 const int data_fd=exclusive(partial_data);
 if(data_fd<0){fs::remove(partial_ledger,ec);error="Cannot exclusively create data staging file";return false;}
 bool okay=true;
 std::vector<unsigned char> buf(storage_audit::block_bytes);
 for(std::size_t i=0;i<ledger.block_hashes.size();i++){
  const auto offset=static_cast<std::uint64_t>(i)*storage_audit::block_bytes;
  const auto length=static_cast<std::size_t>(
    std::min<std::uint64_t>(storage_audit::block_bytes,ledger.bytes-offset));
  if(!read_exact(tls,buf.data(),length)||
     !hashes(buf.data(),length,ledger.block_hashes[i])||
     !fd_write(data_fd,buf.data(),length)){
   okay=false;error="Provisioning interrupted or SHA256 block mismatch";break;
  }
  if(progress && ((i+1)%256==0 || i+1==ledger.block_hashes.size()))
   progress(offset+length,exact_bytes);
 }
 if(!fd_close(data_fd)) {okay=false;error="Could not fsync received sentinel"; }
 std::fill(buf.begin(),buf.end(),0);
 if(!okay){
  fs::remove(partial_data,ec);fs::remove(partial_ledger,ec);return false;
 }
 // No record is admitted until the last hash and the durable disk sync passed.
 // The two-file install is fail-closed, not a distributed atomic transaction.
 if(!is_absent(data_path)||!is_absent(ledger_path)){
  fs::remove(partial_data,ec);fs::remove(partial_ledger,ec);
  error="Destination appeared during transfer";return false;
 }
 fs::rename(partial_data,data_path,ec);
 if(ec){
  fs::remove(partial_data,ec);fs::remove(partial_ledger,ec);
  error="Could not commit complete 13 GiB data";return false;
 }
 fs::rename(partial_ledger,ledger_path,ec);
 if(ec){
  fs::remove(data_path,ec);fs::remove(partial_ledger,ec);
  error="Could not commit ledger; removed incomplete audit installation";return false;
 }
 if(!write_exact(tls,success,sizeof(success))){
  error="Installed verified data but final ACK could not be transmitted";
  return false; // Caller can inspect completed dataset; NEVER re-run blindly.
 }
 return true;
}
}
bool receive_once(const std::string& bind_ipv4,std::uint16_t tcp_port,
 EVP_PKEY* node_key,const std::vector<admission::PublicKey>& auditors,
 const fs::path& data,const fs::path& ledger,
 std::uint64_t exact_bytes,std::string& error,
 const Progress& progress){
 error.clear();
 admission::PublicKey identity{};
 if(!sockets_ready()||auditors.empty()||!get_public(node_key,identity)||
    exact_bytes<storage_audit::block_bytes*storage_audit::challenge_samples||
    exact_bytes>meshcloud::maximum_bytes||data==ledger){
  error="Invalid pinned node/auditor identities, data paths or capacity";return false;
 }
 sockaddr_in addr{};
 if(!address_of(bind_ipv4,tcp_port,addr,true)){
  error="Specify a real LAN IPv4 address, never wildcard";return false;
 }
 auto context=tls_context(node_key,true);
 if(!context){error="Node TLS 1.3 context failed";return false;}
 SocketOwner listener(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));
 if(listener.socket==invalid){error="Cannot open provisioning TCP listener";return false;}
 int yes=1;setsockopt(listener.socket,SOL_SOCKET,SO_REUSEADDR,
#ifdef _WIN32
  reinterpret_cast<const char*>(&yes),
#else
  &yes,
#endif
  sizeof(yes));
 if(bind(listener.socket,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))!=0||
    listen(listener.socket,2)!=0){error="Cannot bind provisioning port";return false;}
 fd_set set{};FD_ZERO(&set);FD_SET(listener.socket,&set);timeval wait{};wait.tv_sec=45;
 if(select(static_cast<int>(listener.socket)+1,&set,nullptr,nullptr,&wait)!=1){
  error="Auditor did not connect within 45 seconds";return false;
 }
 SocketOwner peer(accept(listener.socket,nullptr,nullptr));
 TLSSession tls(SSL_new(context.get()),SSL_free);
 if(peer.socket==invalid||!tls||!transfer_tls(tls.get(),peer.socket,true)||
    !known(tls.get(),auditors)){
  error="Rejected TLS client: auditor identity not pre-pinned";return false;
 }
 const bool complete=ingest(tls.get(),data,ledger,identity,exact_bytes,error,progress);
 if(complete)SSL_shutdown(tls.get());
 return complete;
}
bool send_once(const std::string& ip,std::uint16_t tcp_port,
 EVP_PKEY* auditor_key,const admission::PublicKey& pinned_node,
 const fs::path& source,const fs::path& ledger_path,
 std::uint64_t exact_bytes,std::string& error,
 const Progress& progress){
 error.clear();
 admission::PublicKey auditor{};
 if(!sockets_ready()||!get_public(auditor_key,auditor)||
    pinned_node==admission::PublicKey{}||
    exact_bytes<storage_audit::block_bytes*storage_audit::challenge_samples||
    exact_bytes>meshcloud::maximum_bytes){
  error="Invalid pinned identities or expected storage amount";return false;
 }
 storage_audit::Ledger ledger{};
 std::error_code ec;
 if(!storage_audit::load_ledger(ledger_path,ledger)||
    ledger.bytes!=exact_bytes||ledger.node!=pinned_node ||
    !fs::is_regular_file(source,ec)||ec||
    fs::file_size(source,ec)!=exact_bytes||ec){
  error="13 GiB source, independent ledger and pinned node do not match";
  return false;
 }
 if(fs::file_size(ledger_path,ec)>max_ledger_packet||ec){
  error="Invalid source ledger length";return false;
 }
 std::ifstream archive(ledger_path,std::ios::binary);
 std::vector<unsigned char> encoded(static_cast<std::size_t>(fs::file_size(ledger_path)));
 if(!archive.read(reinterpret_cast<char*>(encoded.data()),
    static_cast<std::streamsize>(encoded.size()))){
  error="Cannot load auditor hash ledger";return false;
 }
 sockaddr_in addr{};
 if(!address_of(ip,tcp_port,addr,false)){error="Invalid node IPv4";return false;}
 auto ctx=tls_context(auditor_key,false);
 if(!ctx){error="Auditor TLS 1.3 context failed";return false;}
 SocketOwner peer(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));
 TLSSession tls(SSL_new(ctx.get()),SSL_free);
 if(peer.socket==invalid||!tls||!set_timeout(peer.socket)||
    connect(peer.socket,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))!=0||
    !transfer_tls(tls.get(),peer.socket,false)||
    !known(tls.get(),{pinned_node})){
  error="TLS node certificate is not the configured Ed25519 key";return false;
 }
 if(!send_small(tls.get(),encoded)){
  error="Could not send authenticated 13 GiB ledger";return false;
 }
 unsigned char acknowledgement[4]{};
 if(!read_exact(tls.get(),acknowledgement,4)||
    !std::equal(acknowledgement,acknowledgement+4,ready)){
  error="Node refused signed/verified dataset configuration";return false;
 }
 std::ifstream file(source,std::ios::binary);
 if(!file){error="Cannot open auditor's provisioned sentinel dataset";return false;}
 std::vector<unsigned char> buf(storage_audit::block_bytes);
 for(std::size_t i=0;i<ledger.block_hashes.size();i++){
  const auto offset=static_cast<std::uint64_t>(i)*storage_audit::block_bytes;
  const auto amount=static_cast<std::size_t>(
    std::min<std::uint64_t>(storage_audit::block_bytes,ledger.bytes-offset));
  if(!file.read(reinterpret_cast<char*>(buf.data()),static_cast<std::streamsize>(amount))||
     !hashes(buf.data(),amount,ledger.block_hashes[i])||
     !write_exact(tls.get(),buf.data(),amount)){
   error="Sending 13 GiB block failed or source hash changed";return false;
  }
  if(progress && ((i+1)%256==0 || i+1==ledger.block_hashes.size()))
   progress(offset+amount,exact_bytes);
 }
 std::fill(buf.begin(),buf.end(),0);
 if(!read_exact(tls.get(),acknowledgement,4)||
    !std::equal(acknowledgement,acknowledgement+4,success)){
  error="No verified final commit acknowledgement from recipient";return false;
 }
 SSL_shutdown(tls.get());
 return true;
}
} // namespace meshcloud::provision
