// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "admission.hpp"
#include <openssl/evp.h>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
using namespace meshcloud;
using namespace meshcloud::admission;
namespace {
using PKey=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
PKey identity() {
 EVP_PKEY_CTX* context=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);
 assert(context);
 assert(EVP_PKEY_keygen_init(context)==1);
 EVP_PKEY* key=nullptr;
 assert(EVP_PKEY_keygen(context,&key)==1);
 EVP_PKEY_CTX_free(context);
 return PKey(key,EVP_PKEY_free);
}
PublicKey pub(EVP_PKEY* key) {
 PublicKey bytes{};std::size_t len=bytes.size();
 assert(EVP_PKEY_get_raw_public_key(key,bytes.data(),&len)==1);
 assert(len==bytes.size());
 return bytes;
}
Signature sign(EVP_PKEY* key,const std::vector<unsigned char>& bytes) {
 Signature signature{};std::size_t len=signature.size();
 EVP_MD_CTX* ctx=EVP_MD_CTX_new();assert(ctx);
 assert(EVP_DigestSignInit(ctx,nullptr,nullptr,nullptr,key)==1);
 assert(EVP_DigestSign(ctx,signature.data(),&len,bytes.data(),bytes.size())==1);
 assert(len==signature.size());EVP_MD_CTX_free(ctx);
 return signature;
}
}
int main() {
 auto peer=identity();
 auto evil=identity();
 std::vector<PKey> judges;
 TrustPolicy trust;trust.quorum=3;
 for(int i=0;i<5;i++) {
  judges.emplace_back(identity());
  trust.pinned_auditors.push_back(pub(judges.back().get()));
 }
 const std::uint64_t now=1800000000;
 MemberTicket ticket{};
 ticket.node_identity=pub(peer.get());
 ticket.ticket_nonce[0]=51;
 ticket.offered_bytes=minimum_bytes;
 ticket.audited_bytes=minimum_bytes;
 ticket.issued_at=now-90;ticket.expires_at=now+100;
 ticket.protocol_version=1;
 for(int i=0;i<3;i++)
  ticket.auditors.push_back({pub(judges[i].get()),
                            sign(judges[i].get(),audit_transcript(ticket))});
 Challenge nonce{},exporter{};
 nonce[0]=30;exporter[0]=31;
 Signature proof=sign(peer.get(),session_transcript(ticket,nonce,exporter));
 VerifiedMembership member{};
 assert(verify_member(ticket,trust,nonce,exporter,proof,now,member));
 const Operation ops[]={Operation::Discovery,Operation::Search,Operation::Listing,
  Operation::Metadata,Operation::Upload,Operation::Download,Operation::Publish,
  Operation::Replication,Operation::Relay};
 for(const auto op:ops)assert(authorize(op,member,now).allowed);
 auto broken=ticket;broken.offered_bytes=minimum_bytes-1;
 assert(!verify_member(broken,trust,nonce,exporter,proof,now,member));
 broken=ticket;broken.audited_bytes=minimum_bytes-1;
 assert(!verify_member(broken,trust,nonce,exporter,proof,now,member));
 broken=ticket;broken.auditors.pop_back();
 assert(!verify_member(broken,trust,nonce,exporter,proof,now,member));
 broken=ticket;broken.auditors.back()=broken.auditors.front();
 assert(!verify_member(broken,trust,nonce,exporter,proof,now,member));
 broken=ticket;broken.auditors[0].signature[0]^=1;
 assert(!verify_member(broken,trust,nonce,exporter,proof,now,member));
 broken=ticket;broken.auditors[1].signer=pub(evil.get());
 assert(!verify_member(broken,trust,nonce,exporter,proof,now,member));
 broken=ticket;broken.expires_at=now;
 assert(!verify_member(broken,trust,nonce,exporter,proof,now,member));
 broken=ticket;broken.expires_at=now+4000;
 assert(!verify_member(broken,trust,nonce,exporter,proof,now,member));
 broken=ticket;broken.issued_at=now+1;
 assert(!verify_member(broken,trust,nonce,exporter,proof,now,member));
 broken=ticket;broken.protocol_version=2;
 assert(!verify_member(broken,trust,nonce,exporter,proof,now,member));
 auto no_binding=exporter;no_binding.fill(0);
 assert(!verify_member(ticket,trust,nonce,no_binding,proof,now,member));
 auto different_binding=exporter;different_binding[3]=3;
 assert(!verify_member(ticket,trust,nonce,different_binding,proof,now,member));
 auto different_challenge=nonce;different_challenge[4]=9;
 assert(!verify_member(ticket,trust,different_challenge,exporter,proof,now,member));
 const auto stolen=sign(evil.get(),session_transcript(ticket,nonce,exporter));
 assert(!verify_member(ticket,trust,nonce,exporter,stolen,now,member));
 TrustPolicy single=trust;single.quorum=1;
 assert(!verify_member(ticket,single,nonce,exporter,proof,now,member));
 TrustPolicy duplicate=trust;duplicate.pinned_auditors[4]=duplicate.pinned_auditors[0];
 assert(!verify_member(ticket,duplicate,nonce,exporter,proof,now,member));
 // No private auditor secrets are part of shipped programs: tests create
 // ephemeral keys so a compromised repo cannot mint production tickets.
 std::cout<<"Three-of-five auditor quorum, challenge binding and fail-closed policy passed\n";
}
