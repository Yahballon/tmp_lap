// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "exchange.hpp"
#include "cid.hpp"
#include <openssl/evp.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>
namespace fs=std::filesystem;
using namespace meshcloud;
using namespace meshcloud::admission;
using namespace meshcloud::exchange;
namespace {
using Secret=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
Secret make_identity() {
 auto* ctx=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);
 assert(ctx && EVP_PKEY_keygen_init(ctx)==1);
 EVP_PKEY* result=nullptr;assert(EVP_PKEY_keygen(ctx,&result)==1);
 EVP_PKEY_CTX_free(ctx);return {result,EVP_PKEY_free};
}
PublicKey pub(EVP_PKEY* key) {
 PublicKey p{};std::size_t len=p.size();
 assert(EVP_PKEY_get_raw_public_key(key,p.data(),&len)==1 && len==p.size());
 return p;
}
Signature sign(EVP_PKEY* key,const std::vector<unsigned char>& msg) {
 Signature out{};std::size_t len=out.size();
 EVP_MD_CTX* ctx=EVP_MD_CTX_new();assert(ctx);
 assert(EVP_DigestSignInit(ctx,nullptr,nullptr,nullptr,key)==1);
 assert(EVP_DigestSign(ctx,out.data(),&len,msg.data(),msg.size())==1);
 EVP_MD_CTX_free(ctx);assert(len==out.size());return out;
}
std::vector<std::uint8_t> fixture(std::uint8_t salt,std::size_t length) {
 std::vector<std::uint8_t> data(36+length,0);
 data[0]='M';data[1]='C';data[2]='B';data[3]='1';
 data[4]=salt;
 for(std::size_t i=0;i<length;i++)
  data[36+i]=static_cast<std::uint8_t>((salt+i)%251);
 const auto size=static_cast<std::uint32_t>(length);
 for(int i=0;i<4;i++)data[32+i]=static_cast<std::uint8_t>((size>>(i*8))&255);
 return data; // Fake envelope for transport tests, not real AEAD ciphertext.
}
Result status(const std::vector<std::uint8_t>& reply,
 const AuthenticatedPeer& member,const Challenge& nonce,const Challenge& exporter,
 std::uint64_t now) {
 wire::Frame frame;
 assert(wire::authenticated_parse(reply,member,nonce,exporter,now,frame));
 assert(frame.operation==Operation::Metadata && frame.payload.size()==1);
 return static_cast<Result>(frame.payload[0]);
}
}
int main() {
 const std::uint64_t time=1800000000;
 auto peer_key=make_identity();
 TrustPolicy trust;std::vector<Secret> auditors;
 for(int i=0;i<5;i++){
  auditors.emplace_back(make_identity());
  trust.pinned_auditors.push_back(pub(auditors.back().get()));
 }
 MemberTicket ticket;
 ticket.node_identity=pub(peer_key.get());
 ticket.ticket_nonce[0]=31;
 ticket.offered_bytes=minimum_bytes;
 ticket.audited_bytes=minimum_bytes;
 ticket.issued_at=time-30;ticket.expires_at=time+360;
 for(int i=0;i<3;i++)
  ticket.auditors.push_back({pub(auditors[i].get()),sign(auditors[i].get(),audit_transcript(ticket))});
 Challenge nonce{},exporter{};nonce[0]=4;exporter[0]=5;
 const auto challenge_signature=sign(peer_key.get(),session_transcript(ticket,nonce,exporter));
 AuthenticatedPeer valid,unknown;
 assert(authenticate_session(ticket,trust,nonce,exporter,challenge_signature,time,valid));
 auto bad_ticket=ticket;bad_ticket.audited_bytes=minimum_bytes-1;
 assert(!authenticate_session(bad_ticket,trust,nonce,exporter,challenge_signature,time,unknown));
 const auto root=fs::temp_directory_path()/
  ("meshcloud-exchange-tests-"+std::to_string(
  std::chrono::steady_clock::now().time_since_epoch().count()));
 fs::create_directories(root);
 {
  SharedShardStore a(root/"node-A",128);
  SharedShardStore b(root/"node-B",128);
  assert(a.open() && b.open());
  PeerGateway A(a),B(b);
  std::uint64_t deleted=0;
  assert(a.prune(time,deleted)&&deleted==0);
  auto c1=fixture(0x42,48);
  auto c2=fixture(0x43,48);
  const auto cid1=cid::from_bytes(c1),cid2=cid::from_bytes(c2);
  std::vector<std::uint8_t> upload,download,reply,got;
  assert(make_put(cid1,c1,upload)&&make_get(cid1,download));
  assert(!A.handle(upload,unknown,nonce,exporter,time,reply) && reply.empty());
  assert(!A.handle(download,unknown,nonce,exporter,time,reply) && reply.empty());
  auto bad_nonce=nonce;bad_nonce[1]=2;
  assert(!A.handle(upload,valid,bad_nonce,exporter,time,reply));
  auto bad_channel=exporter;bad_channel[2]=3;
  assert(!A.handle(upload,valid,nonce,bad_channel,time,reply));
  assert(A.handle(upload,valid,nonce,exporter,time,reply));
  assert(status(reply,valid,nonce,exporter,time)==Result::Ok);
  assert(A.handle(download,valid,nonce,exporter,time,reply));
  assert(read_get_response(reply,cid1,valid,nonce,exporter,time,got)&&got==c1);
  assert(B.handle(upload,valid,nonce,exporter,time,reply));
  assert(status(reply,valid,nonce,exporter,time)==Result::Ok);
  // Repeating the upload is idempotent, and cannot extend TTL.
  assert(B.handle(upload,valid,nonce,exporter,time+10,reply));
  assert(status(reply,valid,nonce,exporter,time+10)==Result::Ok);
  assert(B.handle(download,valid,nonce,exporter,time+10,reply));
  assert(read_get_response(reply,cid1,valid,nonce,exporter,time+10,got));
  auto tampered=reply;tampered.back()^=1;
  assert(!read_get_response(tampered,cid1,valid,nonce,exporter,time+10,got));
  assert(got.empty());
  assert(!read_get_response(reply,cid2,valid,nonce,exporter,time+10,got));
  assert(B.handle(download,valid,nonce,exporter,time+10,reply));
  assert(read_get_response(reply,cid1,valid,nonce,exporter,time+10,got));
  std::vector<std::uint8_t> invalid=upload;invalid.back()^=1;
  assert(!B.handle(invalid,valid,nonce,exporter,time,reply));
  assert(!make_put(cid1,c2,invalid));
  std::vector<std::uint8_t> upload2;
  assert(make_put(cid2,c2,upload2));
  assert(B.handle(upload2,valid,nonce,exporter,time,reply));
  assert(status(reply,valid,nonce,exporter,time)==Result::QuotaExceeded);
  assert(!B.handle(download,valid,nonce,exporter,time+360,reply));
  // Session expires after 360 seconds, even if blocks are still present.
  assert(!B.handle(download,valid,nonce,exporter,time+360,reply));
  assert(b.prune(time+staging_ttl_seconds+1,deleted)&&deleted==1);
  // Integrity-checked shard GETs did not renew the storage lease.
 }
 fs::remove_all(root);
 std::cout<<"Two simulated peers: member-only GET/PUT, CID integrity, quota, no TTL gaming and crash cleanup passed\n";
}
