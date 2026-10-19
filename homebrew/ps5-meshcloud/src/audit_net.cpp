// SPDX-License-Identifier: GPL-3.0-or-later
#include "audit_net.hpp"
#include "core.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <limits>
#include <memory>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif
namespace meshcloud::audit_net {
namespace {
#ifdef _WIN32
using Socket=SOCKET;
constexpr Socket invalid=INVALID_SOCKET;
void close_socket(Socket fd){if(fd!=invalid)closesocket(fd);}
bool socket_ready(){static bool result=[](){WSADATA d{};return WSAStartup(MAKEWORD(2,2),&d)==0;}();return result;}
#else
using Socket=int;
constexpr Socket invalid=-1;
void close_socket(Socket fd){if(fd>=0)close(fd);}
bool socket_ready(){return true;}
#endif
struct Descriptor {
 Socket fd=invalid;
 explicit Descriptor(Socket value=invalid):fd(value){}
 ~Descriptor(){close_socket(fd);}
 Descriptor(const Descriptor&)=delete;
 Descriptor& operator=(const Descriptor&)=delete;
};
using Context=std::unique_ptr<SSL_CTX,decltype(&SSL_CTX_free)>;
using Session=std::unique_ptr<SSL,decltype(&SSL_free)>;
using Cert=std::unique_ptr<X509,decltype(&X509_free)>;
using Key=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
constexpr std::size_t request_bytes=4+32+16+16+8+4+storage_audit::challenge_samples*4;
constexpr std::size_t max_answer=4+16+4+storage_audit::challenge_samples*(4+storage_audit::block_bytes);
void append32(std::vector<unsigned char>& b,std::uint32_t n){
 for(int i=0;i<4;i++)b.push_back(static_cast<unsigned char>((n>>(i*8))&255));
}
void append64(std::vector<unsigned char>& b,std::uint64_t n){
 for(int i=0;i<8;i++)b.push_back(static_cast<unsigned char>((n>>(i*8))&255));
}
bool get32(const std::vector<unsigned char>& b,std::size_t& at,std::uint32_t& n) {
 if(at>b.size()||b.size()-at<4)return false;
 n=0;for(int i=0;i<4;i++)n|=static_cast<std::uint32_t>(b[at++])<<(8*i);
 return true;
}
bool get64(const std::vector<unsigned char>& b,std::size_t& at,std::uint64_t& n) {
 if(at>b.size()||b.size()-at<8)return false;
 n=0;for(int i=0;i<8;i++)n|=static_cast<std::uint64_t>(b[at++])<<(8*i);
 return true;
}
template<std::size_t N>
bool getarray(const std::vector<unsigned char>& b,std::size_t& at,std::array<unsigned char,N>& out) {
 if(at>b.size()||b.size()-at<N)return false;
 std::copy(b.begin()+static_cast<std::ptrdiff_t>(at),
           b.begin()+static_cast<std::ptrdiff_t>(at+N),out.begin());
 at+=N;return true;
}
template<std::size_t N>
void appendarray(std::vector<unsigned char>& b,const std::array<unsigned char,N>& x){
 b.insert(b.end(),x.begin(),x.end());
}
std::uint64_t time_now(){
 const auto secs=std::chrono::duration_cast<std::chrono::seconds>(
  std::chrono::system_clock::now().time_since_epoch()).count();
 return secs>0?static_cast<std::uint64_t>(secs):0;
}
bool public_key(EVP_PKEY* priv,admission::PublicKey& out){
 std::size_t n=out.size();
 return priv&&EVP_PKEY_base_id(priv)==EVP_PKEY_ED25519 &&
  EVP_PKEY_get_raw_public_key(priv,out.data(),&n)==1&&n==out.size();
}
bool timeout(Socket s){
#ifdef _WIN32
 DWORD timeout_ms=12000;
 return setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout_ms),sizeof(timeout_ms))==0&&
        setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&timeout_ms),sizeof(timeout_ms))==0;
#else
 timeval t{};t.tv_sec=12;
 return setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&t,sizeof(t))==0 &&
        setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,&t,sizeof(t))==0;
#endif
}
bool ipv4(const std::string& ip,std::uint16_t port,sockaddr_in& addr,bool listener){
 addr={};addr.sin_family=AF_INET;addr.sin_port=htons(port);
 if(port==0||inet_pton(AF_INET,ip.c_str(),&addr.sin_addr)!=1)return false;
 if(listener && addr.sin_addr.s_addr==INADDR_ANY)return false;
 return true;
}
Context context(EVP_PKEY* key,bool server){
 Context ctx(SSL_CTX_new(TLS_method()),SSL_CTX_free);
 if(!ctx||!key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519 ||
  SSL_CTX_set_min_proto_version(ctx.get(),TLS1_3_VERSION)!=1||
  SSL_CTX_set_max_proto_version(ctx.get(),TLS1_3_VERSION)!=1)return {nullptr,SSL_CTX_free};
 // Explicit pinned Ed25519 cert public keys replace CA chain validation.
 // The callback allows self-signed certs; pinning occurs before ANY proof bytes.
 SSL_CTX_set_verify(ctx.get(),SSL_VERIFY_PEER|
   (server?SSL_VERIFY_FAIL_IF_NO_PEER_CERT:0),[](int,X509_STORE_CTX*){return 1;});
 Cert cert(X509_new(),X509_free);
 std::array<unsigned char,8> serial{};
 if(!cert||RAND_bytes(serial.data(),static_cast<int>(serial.size()))!=1||
    X509_set_version(cert.get(),2)!=1)return {nullptr,SSL_CTX_free};
 unsigned long number=1;
 for(auto x:serial)number=(number<<4)^x;
 if(ASN1_INTEGER_set(X509_get_serialNumber(cert.get()),static_cast<long>(number&0x7fffffff))!=1 ||
    !X509_gmtime_adj(X509_getm_notBefore(cert.get()),-60) ||
    !X509_gmtime_adj(X509_getm_notAfter(cert.get()),60*60*24) ||
    X509_set_pubkey(cert.get(),key)!=1)return {nullptr,SSL_CTX_free};
 auto* name=X509_get_subject_name(cert.get());
 if(!name || X509_NAME_add_entry_by_txt(name,"CN",MBSTRING_ASC,
    reinterpret_cast<const unsigned char*>("MeshCloud auditor"),-1,-1,0)!=1 ||
    X509_set_issuer_name(cert.get(),name)!=1 ||
    X509_sign(cert.get(),key,nullptr)<=0 ||
    SSL_CTX_use_certificate(ctx.get(),cert.get())!=1 ||
    SSL_CTX_use_PrivateKey(ctx.get(),key)!=1 ||
    SSL_CTX_check_private_key(ctx.get())!=1)return {nullptr,SSL_CTX_free};
 SSL_CTX_set_options(ctx.get(),SSL_OP_NO_COMPRESSION);
 return ctx;
}
bool pinned(SSL* tls,const std::vector<admission::PublicKey>& expected) {
 Cert certificate(SSL_get1_peer_certificate(tls),X509_free);
 if(!certificate)return false;
 Key pub(X509_get_pubkey(certificate.get()),EVP_PKEY_free);
 admission::PublicKey seen{};
 if(!public_key(pub.get(),seen))return false;
 return std::find(expected.begin(),expected.end(),seen)!=expected.end();
}
bool sslwrite(SSL* tls,const unsigned char* p,std::size_t n){
 for(std::size_t at=0;at<n;){
  int step=SSL_write(tls,p+at,static_cast<int>(std::min<std::size_t>(n-at,16384)));
  if(step<=0)return false;
  at+=static_cast<std::size_t>(step);
 }
 return true;
}
bool sslread(SSL* tls,unsigned char* p,std::size_t n){
 for(std::size_t at=0;at<n;){
  int step=SSL_read(tls,p+at,static_cast<int>(std::min<std::size_t>(n-at,16384)));
  if(step<=0)return false;
  at+=static_cast<std::size_t>(step);
 }
 return true;
}
bool write_packet(SSL* ssl,const std::vector<unsigned char>& b,std::size_t limit) {
 if(b.empty()||b.size()>limit)return false;
 const auto n=static_cast<std::uint32_t>(b.size());
 unsigned char header[4]{
  static_cast<unsigned char>((n>>24)&255),static_cast<unsigned char>((n>>16)&255),
  static_cast<unsigned char>((n>>8)&255),static_cast<unsigned char>(n&255)};
 return sslwrite(ssl,header,4)&&sslwrite(ssl,b.data(),b.size());
}
bool read_packet(SSL* ssl,std::vector<unsigned char>& b,std::size_t limit){
 b.clear();unsigned char header[4]{};
 if(!sslread(ssl,header,4))return false;
 const auto n=static_cast<std::uint32_t>(header[0])<<24 |
              static_cast<std::uint32_t>(header[1])<<16 |
              static_cast<std::uint32_t>(header[2])<<8|header[3];
 if(n==0||n>limit)return false;
 std::vector<unsigned char> buffer(n);
 if(!sslread(ssl,buffer.data(),buffer.size()))return false;
 b.swap(buffer);return true;
}
bool encode_request(const storage_audit::Challenge& c,std::vector<unsigned char>& out){
 out={'M','C','Q','2'};
 appendarray(out,c.node);appendarray(out,c.dataset_id);
 appendarray(out,c.challenge_id);
 append64(out,c.deadline);append32(out,static_cast<std::uint32_t>(c.indices.size()));
 for(auto n:c.indices)append32(out,n);
 return c.indices.size()==storage_audit::challenge_samples && out.size()==request_bytes;
}
bool decode_request(const std::vector<unsigned char>& in,storage_audit::Challenge& out) {
 out={};if(in.size()!=request_bytes||
  !std::equal(in.begin(),in.begin()+4,"MCQ2"))return false;
 std::size_t at=4;std::uint32_t count=0;
 if(!getarray(in,at,out.node)||!getarray(in,at,out.dataset_id)||
    !getarray(in,at,out.challenge_id)||
    !get64(in,at,out.deadline)||!get32(in,at,count)||
    count!=storage_audit::challenge_samples)return false;
 for(std::uint32_t i=0;i<count;i++){
  std::uint32_t idx=0;if(!get32(in,at,idx))return false;
  if(std::find(out.indices.begin(),out.indices.end(),idx)!=out.indices.end())return false;
  out.indices.push_back(idx);
 }
 return at==in.size()&&out.challenge_id!=storage_audit::Nonce{};
}
bool encode_answer(const storage_audit::Answer& answer,std::vector<unsigned char>& out) {
 out={'M','C','R','2'};appendarray(out,answer.challenge_id);
 append32(out,static_cast<std::uint32_t>(answer.sampled_bytes.size()));
 if(answer.sampled_bytes.size()!=storage_audit::challenge_samples)return false;
 for(const auto& bytes:answer.sampled_bytes){
  if(bytes.empty()||bytes.size()>storage_audit::block_bytes)return false;
  append32(out,static_cast<std::uint32_t>(bytes.size()));
  out.insert(out.end(),bytes.begin(),bytes.end());
 }
 return out.size()<=max_answer;
}
bool decode_answer(const std::vector<unsigned char>& in,storage_audit::Answer& answer){
 answer={};if(in.size()<24 || in.size()>max_answer ||
  !std::equal(in.begin(),in.begin()+4,"MCR2"))return false;
 std::size_t at=4;std::uint32_t count=0;
 if(!getarray(in,at,answer.challenge_id)||!get32(in,at,count)||
    count!=storage_audit::challenge_samples)return false;
 for(std::uint32_t i=0;i<count;i++){
  std::uint32_t len=0;
  if(!get32(in,at,len)||len==0||len>storage_audit::block_bytes||
     at>in.size()||in.size()-at<len)return false;
  answer.sampled_bytes.emplace_back(in.begin()+static_cast<std::ptrdiff_t>(at),
    in.begin()+static_cast<std::ptrdiff_t>(at+len));
  at+=len;
 }
 return at==in.size();
}
bool remote_session(SSL* ssl,Socket fd,bool server){
 return timeout(fd)&&SSL_set_fd(ssl,static_cast<int>(fd))==1 &&
        (server?SSL_accept(ssl):SSL_connect(ssl))==1;
}
}
bool serve_one(const std::string& bind_ip,std::uint16_t tcp_port,
  EVP_PKEY* node_private,const std::vector<admission::PublicKey>& auditors,
  const std::filesystem::path& file,const storage_audit::Ledger& dataset,
  std::string& error){
 error.clear();
 admission::PublicKey self{};
 if(!socket_ready()||auditors.empty()||!public_key(node_private,self)||
    self!=dataset.node||dataset.bytes==0||dataset.block_hashes.empty()){
  error="Invalid auditor allowlist, node identity or provisioned data";return false;
 }
 sockaddr_in address{};
 if(!ipv4(bind_ip,tcp_port,address,true)){error="Bind a specific IPv4 address";return false;}
 auto ssl_ctx=context(node_private,true);
 if(!ssl_ctx){error="Could not initialize audit-only TLS 1.3";return false;}
 Descriptor listener(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));
 if(listener.fd==invalid){error="No audit listening socket";return false;}
 int reuse=1;
 setsockopt(listener.fd,SOL_SOCKET,SO_REUSEADDR,
#ifdef _WIN32
  reinterpret_cast<const char*>(&reuse),
#else
  &reuse,
#endif
  sizeof(reuse));
 if(bind(listener.fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0||
    listen(listener.fd,2)!=0){error="Audit port bind failed";return false;}
 fd_set rfds;FD_ZERO(&rfds);FD_SET(listener.fd,&rfds);timeval tv{};tv.tv_sec=12;
 if(select(static_cast<int>(listener.fd)+1,&rfds,nullptr,nullptr,&tv)!=1){
  error="Audit listener timeout";return false;
 }
 Descriptor connection(accept(listener.fd,nullptr,nullptr));
 Session tls(SSL_new(ssl_ctx.get()),SSL_free);
 if(connection.fd==invalid||!tls || !remote_session(tls.get(),connection.fd,true)||
    !pinned(tls.get(),auditors)){
  error="Unpinned/invalid auditor TLS identity";return false;
 }
 std::vector<unsigned char> request,response;
 storage_audit::Challenge challenge{};
 const auto now=time_now();
 if(!read_packet(tls.get(),request,request_bytes)||
    !decode_request(request,challenge)||
    challenge.node!=dataset.node||challenge.dataset_id!=dataset.dataset_id||
    challenge.deadline<now ||challenge.deadline-now>storage_audit::challenge_lifetime){
  error="Malformed, expired or mismatched audit challenge";return false;
 }
 std::error_code ec;
 if(!std::filesystem::is_regular_file(file,ec)||ec||
    std::filesystem::file_size(file,ec)!=dataset.bytes||ec){
  error="Node has missing or wrong-sized reserved audit data";return false;
 }
 storage_audit::Answer samples{};
 if(!storage_audit::answer(file,challenge,samples)||
    !encode_answer(samples,response)||
    !write_packet(tls.get(),response,max_answer)){
  error="Could not answer encrypted audit challenge";return false;
 }
 SSL_shutdown(tls.get());
 return true;
}
bool challenge_once(const std::string& node_ip,std::uint16_t tcp_port,
  EVP_PKEY* auditor_private,const admission::PublicKey& node,
  const storage_audit::Ledger& ledger,
  storage_audit::VerifiedSample& result,std::string& error){
 error.clear();
 admission::PublicKey auditor{};
 if(!socket_ready()||!public_key(auditor_private,auditor)||ledger.node!=node||
    ledger.block_hashes.empty()) {
  error="Invalid auditor identity or independent ledger";return false;
 }
 sockaddr_in address{};
 if(!ipv4(node_ip,tcp_port,address,false)){error="Invalid node IPv4";return false;}
 auto ctx=context(auditor_private,false);
 if(!ctx){error="Could not construct auditor TLS context";return false;}
 Descriptor connection(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));
 Session tls(ctx?SSL_new(ctx.get()):nullptr,SSL_free);
 if(connection.fd==invalid||!tls||!timeout(connection.fd)||
    connect(connection.fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0||
    !remote_session(tls.get(),connection.fd,false) ||
    !pinned(tls.get(),{node})){
  error="Node TLS authentication or connection failed";return false;
 }
 storage_audit::Challenge challenge{};
 const auto now=time_now();
 std::vector<unsigned char> request,reply;
 if(!storage_audit::make_challenge(ledger,now,challenge)||
    !encode_request(challenge,request)||
    !write_packet(tls.get(),request,request_bytes)||
    !read_packet(tls.get(),reply,max_answer)){
  error="Encrypted challenge request timed out or was denied";return false;
 }
 storage_audit::Answer answer{};
 bool consumed=false;
 if(!decode_answer(reply,answer) ||
    !storage_audit::verify(ledger,challenge,answer,time_now(),consumed,result)){
  error="Remote storage proof failed: bad data, nonce, identity or deadline";return false;
 }
 SSL_shutdown(tls.get());
 return true;
}
} // namespace meshcloud::audit_net
