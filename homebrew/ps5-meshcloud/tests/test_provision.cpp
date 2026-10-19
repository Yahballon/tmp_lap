// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "provision.hpp"
#include "storage_audit.hpp"
#include "core.hpp"
#include <openssl/evp.h>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
namespace fs=std::filesystem;
using namespace meshcloud;
namespace {
using Key=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
Key generate(){
 EVP_PKEY_CTX* ctx=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);
 assert(ctx&&EVP_PKEY_keygen_init(ctx)==1);
 EVP_PKEY* k=nullptr;assert(EVP_PKEY_keygen(ctx,&k)==1);
 EVP_PKEY_CTX_free(ctx);return {k,EVP_PKEY_free};
}
admission::PublicKey pub(EVP_PKEY* k){
 admission::PublicKey p{};std::size_t n=p.size();
 assert(EVP_PKEY_get_raw_public_key(k,p.data(),&n)==1&&n==p.size());
 return p;
}
}
int main(){
 auto recipient=generate(),auditor1=generate(),auditor2=generate(),auditor3=generate(),guest=generate();
 const auto original=fs::temp_directory_path()/
    ("meshcloud-verified-stream-"+std::to_string(
     std::chrono::steady_clock::now().time_since_epoch().count()));
 fs::create_directories(original);
 auto source=original/"13gib-transport-fixture.bin";
 auto source_ledger=original/"auditor.mcal";
 const auto target=original/"received.bin",target_ledger=original/"received.mcal";
 std::string error;
 // CI tests the *same bytes-on-wire* format with 4 MiB, but NO ticket can
 // ever authorize anything below exact 13 GiB. Production CLI fixes 13 GiB.
 constexpr std::uint64_t test_bytes=4ULL*1024*1024;
 assert(storage_audit::provision(source,source_ledger,pub(recipient.get()),
                                  test_bytes,error));
 const auto tcp_port=static_cast<std::uint16_t>(44500+
   std::chrono::steady_clock::now().time_since_epoch().count()%1500);
 auto exercise=[&](EVP_PKEY* sender,bool expected,fs::path output,fs::path ledger){
  std::string server_error,client_error;
  bool server_ok=false;
  std::thread thread([&]{
   server_ok=provision::receive_once("127.0.0.1",tcp_port,
    recipient.get(),{pub(auditor1.get()),pub(auditor2.get()),pub(auditor3.get())},
    output,ledger,test_bytes,server_error);
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  const bool client_ok=provision::send_once("127.0.0.1",tcp_port,
       sender,pub(recipient.get()),source,source_ledger,test_bytes,client_error);
  thread.join();
  if(server_ok!=expected||client_ok!=expected)
   std::cerr<<"source: "<<client_error<<" receiver: "<<server_error<<"\n";
  assert(server_ok==expected);
  assert(client_ok==expected);
 };
 exercise(guest.get(),false,target,target_ledger);
 assert(!fs::exists(target)&&!fs::exists(target_ledger));
 assert(!fs::exists(target.string()+".pending"));
 assert(!fs::exists(target_ledger.string()+".pending"));
 exercise(auditor1.get(),true,target,target_ledger);
 assert(fs::file_size(target)==test_bytes);
 storage_audit::Ledger recovered{};
 assert(storage_audit::load_ledger(target_ledger,recovered));
 assert(recovered.node==pub(recipient.get()));
 storage_audit::Challenge challenge{};
 assert(storage_audit::make_challenge(recovered,1800000000,challenge));
 storage_audit::Answer answer{};
 assert(storage_audit::answer(target,challenge,answer));
 bool consumed=false;
 storage_audit::VerifiedSample proof;
 assert(storage_audit::verify(recovered,challenge,answer,1800000001,consumed,proof));
 // Receiver refuses to replace the existing complete 4 MiB dataset.
 exercise(auditor1.get(),false,target,target_ledger);
 assert(fs::file_size(target)==test_bytes);
 fs::remove_all(original);
 std::cout<<"True pinned TLS streaming: 4MiB SHA256-checked disk transfer; "
             "unknown auditors and replacement denied. Only 13GiB can obtain membership.\n";
}
