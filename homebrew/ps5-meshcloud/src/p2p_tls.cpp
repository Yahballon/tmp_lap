// SPDX-License-Identifier: GPL-3.0-or-later
#include "p2p_tls.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
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
#include <cerrno>
#endif

namespace meshcloud::p2p {
namespace {
#ifdef _WIN32
using Socket=SOCKET;
constexpr Socket bad_socket=INVALID_SOCKET;
void close_socket(Socket s){if(s!=bad_socket)closesocket(s);}
bool socket_init(){
 static const bool ok=[](){WSADATA data{};return WSAStartup(MAKEWORD(2,2),&data)==0;}();
 return ok;
}
#else
using Socket=int;
constexpr Socket bad_socket=-1;
void close_socket(Socket s){if(s>=0)close(s);}
bool socket_init(){return true;}
#endif
struct SocketGuard {
 Socket value=bad_socket;
 explicit SocketGuard(Socket s=bad_socket):value(s){}
 ~SocketGuard(){close_socket(value);}
 SocketGuard(const SocketGuard&)=delete;
 SocketGuard& operator=(const SocketGuard&)=delete;
};
using Context=std::unique_ptr<SSL_CTX,decltype(&SSL_CTX_free)>;
using SSLHandle=std::unique_ptr<SSL,decltype(&SSL_free)>;
using Certificate=std::unique_ptr<X509,decltype(&X509_free)>;
using PKey=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
constexpr char exporter_label[]="EXPORTER-MeshCloud-P2P-v1";
std::uint64_t epoch_now() {
 const auto n=std::chrono::duration_cast<std::chrono::seconds>(
     std::chrono::system_clock::now().time_since_epoch()).count();
 return n>0?static_cast<std::uint64_t>(n):0;
}
bool public_id(EVP_PKEY* key,admission::PublicKey& identity) {
 if(!key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519)return false;
 std::size_t n=identity.size();
 return EVP_PKEY_get_raw_public_key(key,identity.data(),&n)==1 &&
        n==identity.size();
}
bool has_pinned_peer(const admission::PublicKey& identity,
                     const std::vector<admission::PublicKey>& allowlist) {
 return std::find(allowlist.begin(),allowlist.end(),identity)!=allowlist.end();
}
void append32(std::vector<std::uint8_t>& b,std::uint32_t v) {
 for(int i=0;i<4;i++)b.push_back(static_cast<std::uint8_t>((v>>(8*i))&0xff));
}
void append64(std::vector<std::uint8_t>& b,std::uint64_t v) {
 for(int i=0;i<8;i++)b.push_back(static_cast<std::uint8_t>((v>>(8*i))&0xff));
}
bool read32(const std::vector<std::uint8_t>& b,std::size_t& pos,std::uint32_t& val) {
 if(pos>b.size()||b.size()-pos<4)return false;
 val=0;for(int i=0;i<4;i++)val|=static_cast<std::uint32_t>(b[pos++])<<(8*i);
 return true;
}
bool read64(const std::vector<std::uint8_t>& b,std::size_t& pos,std::uint64_t& val) {
 if(pos>b.size()||b.size()-pos<8)return false;
 val=0;for(int i=0;i<8;i++)val|=static_cast<std::uint64_t>(b[pos++])<<(8*i);
 return true;
}
template<std::size_t N>
bool readarray(const std::vector<std::uint8_t>& b,std::size_t& pos,std::array<unsigned char,N>& a) {
 if(pos>b.size()||b.size()-pos<N)return false;
 std::copy(b.begin()+static_cast<std::ptrdiff_t>(pos),
           b.begin()+static_cast<std::ptrdiff_t>(pos+N),a.begin());
 pos+=N;return true;
}
template<std::size_t N>
void appendarray(std::vector<std::uint8_t>& b,const std::array<unsigned char,N>& a) {
 b.insert(b.end(),a.begin(),a.end());
}
// This is an isolated transport encoding, NOT a source of truth.
// Signatures and peer identity get verified separately before dispatch.
bool encode_ticket(const admission::MemberTicket& ticket,
                   const admission::Challenge& nonce,
                   std::vector<std::uint8_t>& encoded) {
 encoded.clear();
 if(ticket.auditors.empty()||ticket.auditors.size()>32)return false;
 encoded.insert(encoded.end(),{'M','C','T','1'});
 appendarray(encoded,ticket.node_identity);
 appendarray(encoded,ticket.ticket_nonce);
 append64(encoded,ticket.offered_bytes);
 append64(encoded,ticket.audited_bytes);
 append64(encoded,ticket.issued_at);
 append64(encoded,ticket.expires_at);
 append32(encoded,ticket.protocol_version);
 encoded.push_back(static_cast<std::uint8_t>(ticket.auditors.size()));
 for(const auto& a:ticket.auditors) {
  appendarray(encoded,a.signer);
  appendarray(encoded,a.signature);
 }
 appendarray(encoded,nonce);
 return encoded.size()<=max_ticket_bytes;
}
bool decode_ticket(const std::vector<std::uint8_t>& data,
                   admission::MemberTicket& ticket,admission::Challenge& challenge) {
 ticket={};challenge={};
 if(data.size()<4+32+16+32+4+1+32||data.size()>max_ticket_bytes||
    !std::equal(data.begin(),data.begin()+4,"MCT1"))return false;
 std::size_t at=4;
 if(!readarray(data,at,ticket.node_identity) ||
    !readarray(data,at,ticket.ticket_nonce) ||
    !read64(data,at,ticket.offered_bytes) ||
    !read64(data,at,ticket.audited_bytes) ||
    !read64(data,at,ticket.issued_at) ||
    !read64(data,at,ticket.expires_at) ||
    !read32(data,at,ticket.protocol_version) || at==data.size())return false;
 const auto count=data[at++];
 if(count<3||count>32 || data.size()-at!=static_cast<std::size_t>(count)*96+32)
  return false;
 for(unsigned i=0;i<count;i++) {
  admission::AuditorSignature a;
  if(!readarray(data,at,a.signer)||!readarray(data,at,a.signature))return false;
  ticket.auditors.push_back(a);
 }
 return readarray(data,at,challenge)&&at==data.size();
}
bool send_exact(SSL* ssl,const std::uint8_t* buffer,std::size_t length) {
 std::size_t offset=0;
 while(offset<length) {
  const auto n=std::min<std::size_t>(length-offset,16*1024);
  const int count=SSL_write(ssl,buffer+offset,static_cast<int>(n));
  if(count<=0)return false;
  offset+=static_cast<std::size_t>(count);
 }
 return true;
}
bool recv_exact(SSL* ssl,std::uint8_t* buffer,std::size_t length) {
 std::size_t at=0;
 while(at<length) {
  const auto want=std::min<std::size_t>(length-at,16*1024);
  const int count=SSL_read(ssl,buffer+at,static_cast<int>(want));
  if(count<=0)return false;
  at+=static_cast<std::size_t>(count);
 }
 return true;
}
bool send_packet(SSL* ssl,const std::vector<std::uint8_t>& data,std::size_t max) {
 if(data.empty()||data.size()>max)return false;
 const auto size=static_cast<std::uint32_t>(data.size());
 const std::array<std::uint8_t,4> length{
  static_cast<std::uint8_t>((size>>24)&255),
  static_cast<std::uint8_t>((size>>16)&255),
  static_cast<std::uint8_t>((size>>8)&255),
  static_cast<std::uint8_t>(size&255)};
 return send_exact(ssl,length.data(),length.size()) &&
        send_exact(ssl,data.data(),data.size());
}
bool receive_packet(SSL* ssl,std::vector<std::uint8_t>& result,std::size_t max) {
 result.clear();
 std::array<std::uint8_t,4> header{};
 if(!recv_exact(ssl,header.data(),header.size()))return false;
 const auto n=(static_cast<std::uint32_t>(header[0])<<24) |
    (static_cast<std::uint32_t>(header[1])<<16) |
    (static_cast<std::uint32_t>(header[2])<<8) | header[3];
 if(n==0||n>max)return false;
 std::vector<std::uint8_t> buffer(n);
 if(!recv_exact(ssl,buffer.data(),buffer.size()))return false;
 result.swap(buffer);return true;
}
bool sign_identity(EVP_PKEY* key,const std::vector<std::uint8_t>& transcript,
                   admission::Signature& proof) {
 proof.fill(0);
 EVP_MD_CTX* ctx=EVP_MD_CTX_new();
 if(!ctx)return false;
 std::size_t length=proof.size();
 const bool ok=EVP_DigestSignInit(ctx,nullptr,nullptr,nullptr,key)==1 &&
    EVP_DigestSign(ctx,proof.data(),&length,transcript.data(),transcript.size())==1 &&
    length==proof.size();
 EVP_MD_CTX_free(ctx);
 return ok;
}
Context make_context(const NodeIdentity& node,bool server) {
 Context ctx(SSL_CTX_new(TLS_method()),SSL_CTX_free);
 if(!ctx || SSL_CTX_set_min_proto_version(ctx.get(),TLS1_3_VERSION)!=1 ||
    SSL_CTX_set_max_proto_version(ctx.get(),TLS1_3_VERSION)!=1)return {nullptr,SSL_CTX_free};
 // Cert chain validation is REPLACED by explicit Ed25519 allowlist pinning.
 // Require a client certificate on the server. Unknown cert -> close before
 // transmitting any MeshCloud ticket, CID, metadata or member operation.
 SSL_CTX_set_verify(ctx.get(),SSL_VERIFY_PEER|
   (server?SSL_VERIFY_FAIL_IF_NO_PEER_CERT:0),
   [](int,X509_STORE_CTX*){return 1;});
 Certificate cert(X509_new(),X509_free);
 if(!cert || X509_set_version(cert.get(),2)!=1 ||
    ASN1_INTEGER_set(X509_get_serialNumber(cert.get()),1)!=1 ||
    !X509_gmtime_adj(X509_getm_notBefore(cert.get()),-60) ||
    !X509_gmtime_adj(X509_getm_notAfter(cert.get()),3600*24*30) ||
    X509_set_pubkey(cert.get(),node.private_key)!=1)return {nullptr,SSL_CTX_free};
 auto* name=X509_get_subject_name(cert.get());
 if(!name ||
    X509_NAME_add_entry_by_txt(name,"CN",MBSTRING_ASC,
          reinterpret_cast<const unsigned char*>("MeshCloud P2P"),-1,-1,0)!=1 ||
    X509_set_issuer_name(cert.get(),name)!=1 ||
    X509_sign(cert.get(),node.private_key,nullptr)<=0 ||
    SSL_CTX_use_certificate(ctx.get(),cert.get())!=1 ||
    SSL_CTX_use_PrivateKey(ctx.get(),node.private_key)!=1 ||
    SSL_CTX_check_private_key(ctx.get())!=1)
   return {nullptr,SSL_CTX_free};
 SSL_CTX_set_options(ctx.get(),SSL_OP_NO_COMPRESSION);
 return ctx;
}
bool check_socket_identity(SSL* ssl,const NodeIdentity& local,
                           admission::PublicKey& peerid) {
 Certificate cert(SSL_get1_peer_certificate(ssl),X509_free);
 if(!cert)return false;
 PKey pub(X509_get_pubkey(cert.get()),EVP_PKEY_free);
 return pub && public_id(pub.get(),peerid) &&
        has_pinned_peer(peerid,local.allowed_peer_keys);
}
bool set_timeout(Socket fd) {
#ifdef _WIN32
 DWORD timeout=6000;
 return setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),
                   sizeof(timeout))==0 &&
        setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&timeout),
                   sizeof(timeout))==0;
#else
 timeval timeout{};timeout.tv_sec=6;
 return setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==0 &&
        setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout))==0;
#endif
}
bool peer_address(const std::string& ip,std::uint16_t port,sockaddr_in& address) {
 address={};address.sin_family=AF_INET;
 address.sin_port=htons(port);
 return port!=0 && inet_pton(AF_INET,ip.c_str(),&address.sin_addr)==1;
}
bool handshake(SSL* tls,const NodeIdentity& local,
               admission::AuthenticatedPeer& peer,
               admission::Challenge& challenge,admission::Challenge& exporter,
               std::string& error) {
 admission::PublicKey pinned{};
 if(!check_socket_identity(tls,local,pinned)){
  error="Peer TLS Ed25519 key not on the pre-approved allowlist";
  return false;
 }
 if(RAND_bytes(challenge.data(),static_cast<int>(challenge.size()))!=1 ||
    SSL_export_keying_material(tls,exporter.data(),exporter.size(),exporter_label,
      sizeof(exporter_label)-1,nullptr,0,0)!=1) {
  error="TLS exporter/secure challenge failure";return false;
 }
 admission::PublicKey own{};
 if(!public_id(local.private_key,own)||own!=local.ticket.node_identity ||
    local.auditors.quorum<3) {
  error="Local membership private key/ticket or quorum invalid";return false;
 }
 std::vector<std::uint8_t> outgoing,incoming;
 if(!encode_ticket(local.ticket,challenge,outgoing) ||
    !send_packet(tls,outgoing,max_ticket_bytes) ||
    !receive_packet(tls,incoming,max_ticket_bytes)){
  error="No valid peer ticket exchange";return false;
 }
 admission::MemberTicket remote_ticket{};
 admission::Challenge their_challenge{};
 Certificate peer_cert(SSL_get1_peer_certificate(tls),X509_free);
 PKey tls_pub(peer_cert?X509_get_pubkey(peer_cert.get()):nullptr,EVP_PKEY_free);
 if(!decode_ticket(incoming,remote_ticket,their_challenge) ||
    !verify_tls_peer_credentials(tls_pub.get(),remote_ticket,local.allowed_peer_keys) ||
    remote_ticket.node_identity!=pinned) {
  error="Remote peer certificate identity and ticket do not match";return false;
 }
 admission::Signature local_sig{};
 const auto signed_data=admission::session_transcript(local.ticket,their_challenge,exporter);
 if(!sign_identity(local.private_key,signed_data,local_sig)) {
  error="Could not sign fresh TLS-bound session";return false;
 }
 std::vector<std::uint8_t> proof(local_sig.begin(),local_sig.end()),remote_sig;
 if(!send_packet(tls,proof,64) || !receive_packet(tls,remote_sig,64) ||
    remote_sig.size()!=64) {
  error="Missing remote Ed25519 session proof";return false;
 }
 admission::Signature their_sig{};
 std::copy(remote_sig.begin(),remote_sig.end(),their_sig.begin());
 const auto now=epoch_now();
 if(!admission::authenticate_session(remote_ticket,local.auditors,
       challenge,exporter,their_sig,now,peer)) {
  error="Remote member denied: missing valid independent 10-GiB audit quorum";
  return false;
 }
 // A local caller must not use a genuine remote ticket to enter with a fake
 // or stale *own* ticket, even if the peer is otherwise valid.
 admission::AuthenticatedPeer self{};
 admission::Signature check{};
 const auto ours=admission::session_transcript(local.ticket,their_challenge,exporter);
 if(!sign_identity(local.private_key,ours,check) ||
    !admission::authenticate_session(local.ticket,local.auditors,
            their_challenge,exporter,check,now,self)) {
  error="Local node lacks a valid signed 10-GiB storage ticket";return false;
 }
 return true;
}
bool tls_link(SSL* ssl,Socket fd,bool server,const NodeIdentity& local,
             admission::AuthenticatedPeer& peer,admission::Challenge& nonce,
             admission::Challenge& exporter,std::string& error) {
 if(!set_timeout(fd)) {error="Cannot apply finite socket timeouts";return false;}
 if(SSL_set_fd(ssl,static_cast<int>(fd))!=1 ||
    (server?SSL_accept(ssl):SSL_connect(ssl))!=1) {
  error="TLS 1.3 peer handshake failed";return false;
 }
 return handshake(ssl,local,peer,nonce,exporter,error);
}
}
bool verify_tls_peer_credentials(EVP_PKEY* tls_pub,
 const admission::MemberTicket& ticket,
 const std::vector<admission::PublicKey>& pinned) {
 if(ticket.node_identity==admission::PublicKey{} ||
    !has_pinned_peer(ticket.node_identity,pinned))return false;
 if(!tls_pub)return false;
 admission::PublicKey tls_id{};
 return public_id(tls_pub,tls_id) && tls_id==ticket.node_identity;
}
bool listen_once(const std::string& bind_ipv4,std::uint16_t port,
 const NodeIdentity& local,exchange::PeerGateway& gateway,
 std::string& error,std::atomic<bool>* stop) {
 error.clear();
 if(local.allowed_peer_keys.empty()||!socket_init()) {
  error="No pinned peer trust keys or sockets unavailable";return false;
 }
 sockaddr_in addr{};
 if(!peer_address(bind_ipv4,port,addr) ||
    addr.sin_addr.s_addr==INADDR_ANY) {
  error="Must bind explicit IPv4 interface (never 0.0.0.0)";return false;
 }
 auto ctx=make_context(local,true);
 if(!ctx){error="Could not initialize Ed25519 TLS1.3 server";return false;}
 SocketGuard listening(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));
 if(listening.value==bad_socket){error="Could not open TCP listening socket";return false;}
 int reuse=1;
 setsockopt(listening.value,SOL_SOCKET,SO_REUSEADDR,
#ifdef _WIN32
    reinterpret_cast<const char*>(&reuse),
#else
    &reuse,
#endif
    sizeof(reuse));
 if(bind(listening.value,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))!=0 ||
    listen(listening.value,4)!=0) {
  error="Could not bind/listen on the selected IPv4 address and port";return false;
 }
 // One-request API, designed for a controlled LAN harness.
 // Callers may run again to accept the next authenticated connection.
 fd_set readfds;FD_ZERO(&readfds);FD_SET(listening.value,&readfds);
 timeval wait{};wait.tv_sec=8;
 const int ready=select(static_cast<int>(listening.value)+1,&readfds,nullptr,nullptr,&wait);
 if(stop && stop->load())return false;
 if(ready!=1){error="Timed out waiting for peer";return false;}
 SocketGuard fd(accept(listening.value,nullptr,nullptr));
 if(fd.value==bad_socket){error="Failed to accept TCP peer";return false;}
 SSLHandle tls(SSL_new(ctx.get()),SSL_free);
 if(!tls){error="Cannot allocate TLS session";return false;}
 admission::AuthenticatedPeer peer{};
 admission::Challenge nonce{},exporter{};
 if(!tls_link(tls.get(),fd.value,true,local,peer,nonce,exporter,error))
  return false;
 std::vector<std::uint8_t> request,response;
 if(!receive_packet(tls.get(),request,max_frame_bytes) ||
    !gateway.handle(request,peer,nonce,exporter,epoch_now(),response) ||
    !send_packet(tls.get(),response,max_frame_bytes)) {
  error="Authenticated frame rejected or transfer failed";return false;
 }
 SSL_shutdown(tls.get());
 return true;
}
bool dial_once(const std::string& remote_ipv4,std::uint16_t port,
 const NodeIdentity& local,const std::vector<std::uint8_t>& request,
 std::vector<std::uint8_t>& response,std::string& error) {
 response.clear();error.clear();
 if(local.allowed_peer_keys.empty()||!socket_init()||request.empty() ||
    request.size()>max_frame_bytes) {
  error="Invalid peer configuration/request";return false;
 }
 sockaddr_in addr{};
 if(!peer_address(remote_ipv4,port,addr)) {
  error="Invalid peer IPv4 address";return false;
 }
 auto ctx=make_context(local,false);
 if(!ctx){error="Could not initialize Ed25519 TLS1.3 client";return false;}
 SocketGuard fd(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));
 if(fd.value==bad_socket||!set_timeout(fd.value)||
    connect(fd.value,reinterpret_cast<const sockaddr*>(&addr),sizeof(addr))!=0) {
  error="Could not connect directly to remote peer";return false;
 }
 SSLHandle tls(SSL_new(ctx.get()),SSL_free);
 if(!tls){error="Could not allocate TLS session";return false;}
 admission::AuthenticatedPeer peer{};
 admission::Challenge nonce{},exporter{};
 if(!tls_link(tls.get(),fd.value,false,local,peer,nonce,exporter,error))return false;
 // The gateway will verify authorization on the server. The client must
 // also refuse invalid local request opcodes before transmitting anything.
 wire::Frame parsed;
 if(!wire::authenticated_parse(request,peer,nonce,exporter,epoch_now(),parsed)) {
  error="Local operation was not authorized for audited membership";return false;
 }
 if(!send_packet(tls.get(),request,max_frame_bytes) ||
    !receive_packet(tls.get(),response,max_frame_bytes)) {
  response.clear();error="Remote peer rejected or transfer interrupted";return false;
 }
 if(!wire::authenticated_parse(response,peer,nonce,exporter,epoch_now(),parsed)) {
  response.clear();error="Unauthenticated response";return false;
 }
 SSL_shutdown(tls.get());
 return true;
}
} // namespace meshcloud::p2p
