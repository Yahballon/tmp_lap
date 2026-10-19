// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "p2p_tls.hpp"
#include "cid.hpp"
#include <openssl/evp.h>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
namespace fs=std::filesystem;
using namespace meshcloud;
using namespace meshcloud::p2p;
using namespace meshcloud::admission;
namespace {
using Private=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
Private generate() {
 auto* ctx=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);assert(ctx);
 assert(EVP_PKEY_keygen_init(ctx)==1);
 EVP_PKEY* key=nullptr;assert(EVP_PKEY_keygen(ctx,&key)==1);
 EVP_PKEY_CTX_free(ctx);
 return Private(key,EVP_PKEY_free);
}
PublicKey pub(EVP_PKEY* key) {
 PublicKey out{};std::size_t size=out.size();
 assert(EVP_PKEY_get_raw_public_key(key,out.data(),&size)==1);
 assert(size==out.size());
 return out;
}
Signature sign(EVP_PKEY* key,const std::vector<unsigned char>& data) {
 Signature sig{};std::size_t size=sig.size();
 EVP_MD_CTX* ctx=EVP_MD_CTX_new();assert(ctx);
 assert(EVP_DigestSignInit(ctx,nullptr,nullptr,nullptr,key)==1);
 assert(EVP_DigestSign(ctx,sig.data(),&size,data.data(),data.size())==1);
 assert(size==sig.size());EVP_MD_CTX_free(ctx);
 return sig;
}
std::vector<std::uint8_t> sealed_fixture() {
 // Simulated opaque MCB1 ciphertext. Does NOT prove AES-GCM correctness.
 std::vector<std::uint8_t> bytes(36+256,0);
 bytes[0]='M';bytes[1]='C';bytes[2]='B';bytes[3]='1';
 bytes[32]=0;bytes[33]=1;
 for(std::size_t i=36;i<bytes.size();i++)bytes[i]=static_cast<std::uint8_t>((i*19)%251);
 return bytes;
}
MemberTicket ticket(EVP_PKEY* id,const std::vector<Private>& auditors,std::uint64_t now) {
 MemberTicket t{};
 t.node_identity=pub(id);t.ticket_nonce[0]=41;
 t.offered_bytes=minimum_bytes;t.audited_bytes=minimum_bytes;
 t.issued_at=now-20;t.expires_at=now+300;
 for(int i=0;i<3;i++) {
  t.auditors.push_back({pub(auditors[static_cast<std::size_t>(i)].get()),
    sign(auditors[static_cast<std::size_t>(i)].get(),audit_transcript(t))});
 }
 return t;
}
}
int main(){
 const auto now=static_cast<std::uint64_t>(std::chrono::system_clock::to_time_t(
                                           std::chrono::system_clock::now()));
 auto alice_key=generate(),bob_key=generate(),stranger_key=generate();
 std::vector<Private> auditors;
 for(int i=0;i<5;i++)auditors.push_back(generate());
 TrustPolicy policy;policy.quorum=3;
 for(const auto& k:auditors)policy.pinned_auditors.push_back(pub(k.get()));
 NodeIdentity alice{alice_key.get(),ticket(alice_key.get(),auditors,now),policy,{pub(bob_key.get())}};
 NodeIdentity bob{bob_key.get(),ticket(bob_key.get(),auditors,now),policy,{pub(alice_key.get())}};
 NodeIdentity outsider{stranger_key.get(),ticket(stranger_key.get(),auditors,now),policy,
                       {pub(alice_key.get())}};
 const auto root=fs::temp_directory_path()/
     ("meshcloud-p2p-real-socket-"+std::to_string(
       std::chrono::steady_clock::now().time_since_epoch().count()));
 const auto cid=cid::from_bytes(sealed_fixture());
 {
  exchange::SharedShardStore alice_store(root/"alice",minimum_bytes);
  exchange::SharedShardStore bob_store(root/"bob",minimum_bytes);
  assert(alice_store.open()&&bob_store.open());
  exchange::PeerGateway ag(alice_store),bg(bob_store);
  const auto port=static_cast<std::uint16_t>(
     41000+(std::chrono::steady_clock::now().time_since_epoch().count()%1000));
  std::vector<std::uint8_t> put,get;
  assert(exchange::make_put(cid,sealed_fixture(),put));
  assert(exchange::make_get(cid,get));
  auto roundtrip=[&](NodeIdentity& listener,exchange::PeerGateway& gateway,
                     NodeIdentity& dialer,const std::vector<std::uint8_t>& request,
                     bool expected_ok,std::vector<std::uint8_t>& received) {
   std::string server_error;
   bool server_ok=false;
   std::thread worker([&](){
    server_ok=listen_once("127.0.0.1",port,listener,gateway,server_error);
   });
   std::this_thread::sleep_for(std::chrono::milliseconds(120));
   std::string client_error;
   const bool client_ok=dial_once("127.0.0.1",port,dialer,request,received,client_error);
   worker.join();
   if(client_ok!=expected_ok || server_ok!=expected_ok) {
    std::cerr<<"Client: "<<client_error<<" Server: "<<server_error<<"\n";
   }
   assert(client_ok==expected_ok && server_ok==expected_ok);
  };
  std::vector<std::uint8_t> response;
  roundtrip(alice,ag,bob,put,true,response);
  roundtrip(bob,bg,alice,put,true,response);
  roundtrip(alice,ag,bob,get,true,response);
  // An authenticated remote response is revalidated by CID and ciphertext hash.
  assert(response.size()>36+59);
  auto modified=response;modified.back()^=1;
  assert(!std::equal(response.begin(),response.end(),modified.begin()));
  roundtrip(bob,bg,alice,get,true,response);
  // Even with three stolen mock auditor signatures, a TLS private key not on
  // the allowlist is rejected before publishing or receiving metadata.
  roundtrip(alice,ag,outsider,get,false,response);
  // Peer with unapproved 9-GiB contribution cannot pass signed quorum gate.
  auto denied=bob;
  denied.ticket.audited_bytes=minimum_bytes-1;
  roundtrip(alice,ag,denied,get,false,response);
 }
 fs::remove_all(root);
 std::cout<<"Real loopback TCP+TLS1.3: A<->B upload/download, pinning and 10GiB quorum admission passed\n";
}
