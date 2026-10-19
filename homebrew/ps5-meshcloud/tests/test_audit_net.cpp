// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "audit_net.hpp"
#include "storage_audit.hpp"
#include "core.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <openssl/evp.h>
namespace fs=std::filesystem;
using namespace meshcloud;
using namespace meshcloud::storage_audit;
namespace {
using Key=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
Key generate(){
 auto* ctx=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);
 assert(ctx&&EVP_PKEY_keygen_init(ctx)==1);
 EVP_PKEY* key=nullptr;assert(EVP_PKEY_keygen(ctx,&key)==1);
 EVP_PKEY_CTX_free(ctx);return Key(key,EVP_PKEY_free);
}
admission::PublicKey pub(EVP_PKEY* key){
 admission::PublicKey result{};std::size_t n=result.size();
 assert(EVP_PKEY_get_raw_public_key(key,result.data(),&n)==1&&n==result.size());
 return result;
}
}
int main(){
 const auto root=fs::temp_directory_path()/
   ("meshcloud-audit-tls-"+std::to_string(
     std::chrono::steady_clock::now().time_since_epoch().count()));
 fs::create_directories(root);
 auto node=generate(),judge=generate(),outsider=generate();
 const auto id=pub(node.get());
 std::string error;
 assert(provision(root/"node-data.bin",root/"ledger.mcal",id,4ULL*1024*1024,error));
 Ledger ledger{};
 assert(load_ledger(root/"ledger.mcal",ledger));
 const std::uint16_t port=static_cast<std::uint16_t>(43000+
  std::chrono::steady_clock::now().time_since_epoch().count()%1800);
 auto once=[&](EVP_PKEY* signer,const Ledger& supplied,bool expected){
  bool server_ok=false;
  std::string server_error;
  std::thread server([&]{
   server_ok=audit_net::serve_one("127.0.0.1",port,node.get(),
     {pub(judge.get())},root/"node-data.bin",ledger,server_error);
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  VerifiedSample verified;
  std::string client_error;
  const bool client_ok=audit_net::challenge_once("127.0.0.1",port,
    signer,id,supplied,verified,client_error);
  server.join();
  if(expected!=client_ok ||expected!=server_ok)
   std::cerr<<"Auditor="<<client_error<<" node="<<server_error<<"\n";
  assert(client_ok==expected&&server_ok==expected);
  // A 4 MiB data sample can NEVER authorize a production 13 GiB ticket.
  admission::MemberTicket candidate{};
  candidate.node_identity=id;candidate.ticket_nonce[0]=1;
  candidate.offered_bytes=minimum_bytes;candidate.audited_bytes=minimum_bytes;
  const auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  candidate.issued_at=static_cast<std::uint64_t>(now);
  candidate.expires_at=candidate.issued_at+900;
  admission::AuditorSignature ticket{};
  assert(!sign_ticket_after_proof(judge.get(),ledger,candidate,verified,ticket));
 };
 once(judge.get(),ledger,true);
 once(outsider.get(),ledger,false); // unknown auditor can't read even one block
 auto mismatched=ledger;
 mismatched.dataset_id[1]^=0x41;
 once(judge.get(),mismatched,false);
 fs::remove_all(root);
 std::cout<<"True TLS1.3 audit-only socket: verified 4MiB challenge, certificate pin, bad dataset denial, 13GiB admission lock\n";
}
