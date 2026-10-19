// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "storage_audit.hpp"
#include "core.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <openssl/evp.h>
namespace fs=std::filesystem;
using namespace meshcloud;
using namespace meshcloud::storage_audit;
int main(){
 const auto dir=fs::temp_directory_path()/
  ("meshcloud-audit-"+std::to_string(
    std::chrono::steady_clock::now().time_since_epoch().count()));
 fs::create_directories(dir);
 admission::PublicKey owner{};owner[0]=17;
 const auto data=dir/"node-4mib-random.bin";
 const auto ledger_file=dir/"independent-auditor-ledger.bin";
 std::string error;
 assert(provision(data,ledger_file,owner,4ULL*1024*1024,error));
 assert(fs::file_size(data)==4ULL*1024*1024);
 assert(fs::file_size(ledger_file)==4+1+32+16+8+4+4+64*32);
 assert(!provision(data,ledger_file,owner,4ULL*1024*1024,error));
 // An attacker-controlled dangling symlink must not redirect a random audit
 // write to an arbitrary future file path.
 std::error_code link_error;
 const auto redirect=dir/"dangling-alias";
 const auto victim=dir/"victim-created-by-following-symlink.bin";
 fs::create_symlink(victim,redirect,link_error);
 if(!link_error){
  assert(!provision(redirect,dir/"new-ledger.bin",owner,4ULL*1024*1024,error));
  assert(!fs::exists(victim));
 }
 Ledger ledger{};
 assert(load_ledger(ledger_file,ledger));
 assert(ledger.bytes==4ULL*1024*1024&&ledger.block_hashes.size()==64);
 const std::uint64_t now=1800000000;
 Challenge c{};assert(make_challenge(ledger,now,c));
 Answer reply{};assert(answer(data,c,reply));
 bool consumed=false;VerifiedSample valid;
 assert(verify(ledger,c,reply,now+5,consumed,valid));
 assert(consumed&&!verify(ledger,c,reply,now+5,consumed,valid));
 consumed=false;
 assert(!verify(ledger,c,reply,now+61,consumed,valid));
 consumed=false;auto forged=reply;
 forged.sampled_bytes[0][0]^=1;
 assert(!verify(ledger,c,forged,now+5,consumed,valid));
 assert(consumed&&!verify(ledger,c,reply,now+5,consumed,valid));
 consumed=false;auto modified=c;
 modified.dataset_id[0]^=1;
 assert(!verify(ledger,modified,reply,now+5,consumed,valid));
 modified=c;modified.node[0]^=1;
 assert(!verify(ledger,modified,reply,now+5,consumed,valid));
 modified=c;modified.indices[1]=modified.indices[0];
 assert(!verify(ledger,modified,reply,now+5,consumed,valid));
 admission::MemberTicket ticket{};
 ticket.node_identity=owner;
 ticket.offered_bytes=minimum_bytes;
 ticket.audited_bytes=minimum_bytes;
 ticket.ticket_nonce[0]=1;
 ticket.issued_at=now+5;ticket.expires_at=now+1800;
 EVP_PKEY_CTX* ctx=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);
 assert(ctx&&EVP_PKEY_keygen_init(ctx)==1);
 EVP_PKEY* key=nullptr;assert(EVP_PKEY_keygen(ctx,&key)==1);
 EVP_PKEY_CTX_free(ctx);
 admission::AuditorSignature signature{};
 VerifiedSample empty;
 // A 4 MiB demonstrator NEVER gets a 10 GiB member ticket.
 assert(!sign_ticket_after_proof(key,ledger,ticket,valid,signature));
 assert(!sign_ticket_after_proof(key,ledger,ticket,empty,signature));
 EVP_PKEY_free(key);
 fs::remove_all(dir);
 std::cout<<"CSPRNG provisioning, disk samples, tampering, deadline, one-time proof and 10GiB admission gate passed\n";
}
