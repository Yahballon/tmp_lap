// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "wire.hpp"
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
PKey make_key() {
 EVP_PKEY_CTX* c=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);
 assert(c&&EVP_PKEY_keygen_init(c)==1);
 EVP_PKEY* raw=nullptr;assert(EVP_PKEY_keygen(c,&raw)==1);
 EVP_PKEY_CTX_free(c);return PKey(raw,EVP_PKEY_free);
}
PublicKey public_key(EVP_PKEY* k) {
 PublicKey p{};std::size_t size=p.size();
 assert(EVP_PKEY_get_raw_public_key(k,p.data(),&size)==1&&size==32);return p;
}
Signature signed_bytes(EVP_PKEY* key,const std::vector<unsigned char>& bytes) {
 Signature signature{};std::size_t size=signature.size();
 EVP_MD_CTX* c=EVP_MD_CTX_new();assert(c);
 assert(EVP_DigestSignInit(c,nullptr,nullptr,nullptr,key)==1);
 assert(EVP_DigestSign(c,signature.data(),&size,bytes.data(),bytes.size())==1);
 EVP_MD_CTX_free(c);assert(size==64);return signature;
}
}
int main() {
 const std::uint64_t now=1800000000;
 auto peerkey=make_key();
 TrustPolicy trust;trust.quorum=3;
 std::vector<PKey> auditor;
 MemberTicket ticket{};
 ticket.node_identity=public_key(peerkey.get());
 ticket.ticket_nonce[0]=4;
 ticket.offered_bytes=minimum_bytes;ticket.audited_bytes=minimum_bytes;
 ticket.issued_at=now-30;ticket.expires_at=now+120;
 for(int i=0;i<4;i++) {
  auditor.emplace_back(make_key());
  trust.pinned_auditors.push_back(public_key(auditor.back().get()));
 }
 for(int i=0;i<3;i++)ticket.auditors.push_back({
  public_key(auditor[i].get()),signed_bytes(auditor[i].get(),audit_transcript(ticket))});
 Challenge nonce{},exporter{};nonce[0]=9;exporter[0]=8;
 const auto proof=signed_bytes(peerkey.get(),session_transcript(ticket,nonce,exporter));
 AuthenticatedPeer verified;
 assert(authenticate_session(ticket,trust,nonce,exporter,proof,now,verified));
 AuthenticatedPeer fabricated; // Impossible to set arbitrary admission booleans.
 const Operation every[]={Operation::Discovery,Operation::Search,
  Operation::Listing,Operation::Metadata,Operation::Upload,Operation::Download,
  Operation::Publish,Operation::Replication,Operation::Relay};
 for(const auto op:every) {
  wire::Frame request{op,{1,2,3,4}};
  std::vector<std::uint8_t> encoded;
  assert(wire::serialize(request,encoded));
  wire::Frame received{};
  assert(!wire::authenticated_parse(encoded,fabricated,nonce,exporter,now,received));
  assert(wire::authenticated_parse(encoded,verified,nonce,exporter,now,received));
  assert(received.operation==op&&received.payload==request.payload);
  auto corrupted=encoded;corrupted[4]=9;
  assert(!wire::authenticated_parse(corrupted,verified,nonce,exporter,now,received));
  corrupted=encoded;corrupted[6]=1;
  assert(!wire::authenticated_parse(corrupted,verified,nonce,exporter,now,received));
  corrupted=encoded;corrupted[5]=99;
  assert(!wire::authenticated_parse(corrupted,verified,nonce,exporter,now,received));
  corrupted=encoded;corrupted[11]+=1;
  assert(!wire::authenticated_parse(corrupted,verified,nonce,exporter,now,received));
  corrupted=encoded;corrupted.pop_back();
  assert(!wire::authenticated_parse(corrupted,verified,nonce,exporter,now,received));
 }
 wire::Frame request{Operation::Download,{7}};
 std::vector<std::uint8_t> encoded;
 assert(wire::serialize(request,encoded));
 assert(!wire::authenticated_parse(encoded,verified,nonce,exporter,now+120,request));
 auto other=nonce;other[1]=1;
 assert(!wire::authenticated_parse(encoded,verified,other,exporter,now,request));
 other=exporter;other[1]=1;
 assert(!wire::authenticated_parse(encoded,verified,nonce,other,now,request));
 wire::Frame max{Operation::Upload,std::vector<std::uint8_t>(wire::max_payload_bytes,17)};
 assert(wire::serialize(max,encoded));
 wire::Frame decoded{};
 assert(wire::authenticated_parse(encoded,verified,nonce,exporter,now,decoded));
 max.payload.push_back(0);
 assert(!wire::serialize(max,encoded));
 std::cout<<"Quorum-authenticated, channel-bound member-only frames and bounds passed\n";
}
