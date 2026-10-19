// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ticket_issue.hpp"
#include "p2p_config.hpp"
#include <openssl/evp.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
using namespace meshcloud;
namespace {
using Key=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
Key generate(){
 auto* ctx=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);
 assert(ctx&&EVP_PKEY_keygen_init(ctx)==1);
 EVP_PKEY* key=nullptr;
 assert(EVP_PKEY_keygen(ctx,&key)==1);
 EVP_PKEY_CTX_free(ctx);return {key,EVP_PKEY_free};
}
admission::PublicKey pub(EVP_PKEY* key){
 admission::PublicKey result{};std::size_t n=result.size();
 assert(EVP_PKEY_get_raw_public_key(key,result.data(),&n)==1);
 assert(n==result.size());return result;
}
admission::Signature sign(EVP_PKEY* key,const std::vector<unsigned char>& msg){
 admission::Signature sig{};std::size_t n=sig.size();
 EVP_MD_CTX* ctx=EVP_MD_CTX_new();assert(ctx);
 assert(EVP_DigestSignInit(ctx,nullptr,nullptr,nullptr,key)==1);
 assert(EVP_DigestSign(ctx,sig.data(),&n,msg.data(),msg.size())==1);
 EVP_MD_CTX_free(ctx);assert(n==sig.size());return sig;
}
}
int main(){
 const std::uint64_t now=1800000000;
 auto owner=generate(),stranger=generate();
 std::vector<Key> auditors;
 admission::TrustPolicy trust{};trust.quorum=3;
 for(int i=0;i<5;i++){
  auditors.push_back(generate());
  trust.pinned_auditors.push_back(pub(auditors.back().get()));
 }
 ticket_issue::Request req{};
 assert(ticket_issue::request(owner.get(),now,req));
 assert(req.ticket.node_identity==pub(owner.get()));
 assert(req.ticket.offered_bytes==minimum_bytes&&req.ticket.audited_bytes==minimum_bytes);
 assert(ticket_issue::verify_request(req,now));
 assert(!ticket_issue::verify_request(req,now+601));
 std::vector<unsigned char> bytes;
 assert(ticket_issue::encode_request(req,bytes));
 ticket_issue::Request recovered{};
 assert(ticket_issue::decode_request(bytes,recovered));
 assert(ticket_issue::verify_request(recovered,now));
 auto invalid=bytes;invalid.back()^=1;
 ticket_issue::Request damaged{};
 assert(ticket_issue::decode_request(invalid,damaged));
 assert(!ticket_issue::verify_request(damaged,now));
 invalid=bytes;invalid[52]^=1;
 assert(ticket_issue::decode_request(invalid,damaged));
 assert(!ticket_issue::verify_request(damaged,now));
 auto reissued=req;
 reissued.ticket.ticket_nonce[0]^=1;
 assert(!ticket_issue::verify_request(reissued,now));
 reissued=req;reissued.ticket.node_identity=pub(stranger.get());
 assert(!ticket_issue::verify_request(reissued,now));
 reissued=req;reissued.ticket.offered_bytes=minimum_bytes-1;
 assert(!ticket_issue::verify_request(reissued,now));
 reissued=req;reissued.ticket.audited_bytes=maximum_bytes+1;
 assert(!ticket_issue::verify_request(reissued,now));
 std::vector<ticket_issue::Partial> parts;
 for(int i=0;i<3;i++){
  ticket_issue::Partial part{};
  part.signer=pub(auditors[i].get());
  part.signature=sign(auditors[i].get(),admission::audit_transcript(req.ticket));
  // Transport-level part request hashing uses the owner's signed transcript
  // (not the signature); the SHA-256 transcript is public, not forgeable proof.
  // Compute via successful verify against a stored request file once below.
  const auto msg=admission::audit_transcript(req.ticket);
  const std::string tag="MeshCloud/owner-ticket-request/v1";
  std::vector<unsigned char> owner_msg(tag.begin(),tag.end());
  owner_msg.insert(owner_msg.end(),msg.begin(),msg.end());
  unsigned n=0;
  assert(EVP_Digest(owner_msg.data(),owner_msg.size(),part.request_sha256.data(),&n,
                    EVP_sha256(),nullptr)==1&&n==32);
  assert(ticket_issue::verify_partial(req,part));
  assert(ticket_issue::encode_partial(part,bytes));
  ticket_issue::Partial read{};
  assert(ticket_issue::decode_partial(bytes,read));
  assert(ticket_issue::verify_partial(req,read));
  parts.push_back(read);
 }
 admission::MemberTicket complete{};
 assert(!ticket_issue::assemble(req,{parts[0],parts[1]},trust,now,complete));
 assert(ticket_issue::assemble(req,parts,trust,now,complete));
 assert(complete.auditors.size()==3&&complete.offered_bytes==minimum_bytes);
 std::vector<std::uint8_t> serialized;
 assert(p2p::serialize_member_ticket(complete,serialized));
 admission::MemberTicket restored{};
 assert(p2p::parse_member_ticket(serialized,restored));
 assert(restored.auditors.size()==3);
 assert(restored.ticket_nonce==req.ticket.ticket_nonce);
 assert(!ticket_issue::assemble(req,{parts[0],parts[0],parts[2]},trust,now,complete));
 assert(!ticket_issue::assemble(req,parts,trust,now+601,complete));
 auto unknown=parts;unknown[0].signer=pub(stranger.get());
 assert(!ticket_issue::assemble(req,unknown,trust,now,complete));
 auto tamper=parts;tamper[1].signature[5]^=1;
 assert(!ticket_issue::assemble(req,tamper,trust,now,complete));
 tamper=parts;tamper[1].request_sha256[0]^=1;
 assert(!ticket_issue::assemble(req,tamper,trust,now,complete));
 auto changed=req;changed.ticket.expires_at--;
 assert(!ticket_issue::assemble(changed,parts,trust,now,complete));
 storage_audit::VerifiedSample no_proof{};
 storage_audit::Ledger tiny{};tiny.node=req.ticket.node_identity;tiny.bytes=4096;
 ticket_issue::Partial denied{};
 assert(!ticket_issue::attest(auditors[0].get(),req,tiny,no_proof,now,denied));
 std::cout<<"Owner-signed 13GiB membership request, anchored three-auditor quorum assembly, TTL/tamper/replay defenses passed\n";
}
