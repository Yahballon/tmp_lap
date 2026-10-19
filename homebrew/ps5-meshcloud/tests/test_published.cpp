// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "published.hpp"
#include "exchange.hpp"
#include "cid.hpp"
#include <openssl/evp.h>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <memory>
#include <vector>
#include <iostream>

namespace fs=std::filesystem;
using namespace meshcloud;
using namespace meshcloud::admission;
using namespace meshcloud::published;
namespace {
using Key=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
Key keygen() {
 auto* ctx=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);
 assert(ctx && EVP_PKEY_keygen_init(ctx)==1);
 EVP_PKEY* private_key=nullptr;
 assert(EVP_PKEY_keygen(ctx,&private_key)==1);
 EVP_PKEY_CTX_free(ctx);return {private_key,EVP_PKEY_free};
}
PublicKey public_key(EVP_PKEY* key) {
 PublicKey p{};std::size_t n=p.size();
 assert(EVP_PKEY_get_raw_public_key(key,p.data(),&n)==1&&n==p.size());return p;
}
Signature sign_bytes(EVP_PKEY* key,const std::vector<unsigned char>& data) {
 Signature signature{};std::size_t len=signature.size();
 auto* ctx=EVP_MD_CTX_new();assert(ctx);
 assert(EVP_DigestSignInit(ctx,nullptr,nullptr,nullptr,key)==1);
 assert(EVP_DigestSign(ctx,signature.data(),&len,data.data(),data.size())==1);
 assert(len==signature.size());EVP_MD_CTX_free(ctx);return signature;
}
std::vector<std::uint8_t> ciphertext_fixture(unsigned seed,unsigned size) {
 std::vector<std::uint8_t> data(36+size);
 data[0]='M';data[1]='C';data[2]='B';data[3]='1';
 for(unsigned i=0;i<size;i++)data[36+i]=static_cast<std::uint8_t>((i+seed)%251);
 for(int i=0;i<4;i++)data[32+i]=static_cast<std::uint8_t>((size>>(8*i))&255);
 return data; // Transport-format fixture only; not AES-GCM authenticated ciphertext.
}
}
int main() {
 const std::uint64_t now=1800000000ULL;
 auto author=keygen(),other=keygen(),node=keygen();
 const auto author_pub=public_key(author.get());
 const auto bad_pub=public_key(other.get());
 auto block_a=ciphertext_fixture(15,92),block_b=ciphertext_fixture(23,126);
 const auto cid_a=cid::from_bytes(block_a),cid_b=cid::from_bytes(block_b);
 const auto local_manifest_id=cid::from_bytes(std::vector<std::uint8_t>{55,66,77});
 const std::uint64_t total=block_a.size()+block_b.size();
 Page one;
 one.file_manifest_cid=local_manifest_id;
 one.created_at=now-100;
 one.issued_at=now-10;
 one.expires_at=one.issued_at+page_retention_seconds;
 one.ciphertext_bytes=total;
 one.index=0;one.page_count=2;
 one.chunks={{cid_a,static_cast<std::uint32_t>(block_a.size())}};
 auto two=one;
 two.index=1;two.chunks={{cid_b,static_cast<std::uint32_t>(block_b.size())}};
 assert(sign(one,author.get())&&sign(two,author.get()));
 assert(verify(one,author_pub,now)&&verify(two,author_pub,now));
 assert(!verify(one,bad_pub,now));
 assert(!verify(one,author_pub,one.expires_at));
 assert(!verify(one,author_pub,one.issued_at-1));
 std::vector<Page> pages{one,two};
 assert(verify_complete(pages,author_pub,now));
 std::vector<ChunkRef> refs;
 assert(flatten(pages,author_pub,now,refs));
 assert(refs.size()==2&&refs[0].cid==cid_a&&refs[1].cid==cid_b);
 auto tampered=pages;
 tampered[0].chunks[0].cid=cid_b;
 assert(!verify_complete(tampered,author_pub,now));
 tampered=pages;tampered[1].ciphertext_bytes++;
 assert(!verify_complete(tampered,author_pub,now));
 tampered=pages;tampered[1].signature[0]^=1;
 assert(!verify_complete(tampered,author_pub,now));
 tampered=pages;tampered[1].index=0;
 assert(!verify_complete(tampered,author_pub,now));
 tampered=pages;tampered.pop_back();
 assert(!verify_complete(tampered,author_pub,now));
 tampered=pages;std::swap(tampered[0],tampered[1]);
 assert(!verify_complete(tampered,author_pub,now));
 tampered=pages;tampered[0].issued_at=now+1;
 assert(!verify_complete(tampered,author_pub,now));
 // Publisher renewal is a fresh signed page set (not a lookup/partial download).
 auto renewed=pages;
 for(auto& page:renewed) {
  page.issued_at=now+50;
  page.expires_at=page.issued_at+page_retention_seconds;
  assert(sign(page,author.get()));
 }
 assert(verify_complete(renewed,author_pub,now+51));
 assert(!verify_complete(pages,author_pub,one.expires_at));
 // Real cross-module path: signer CID page -> membership -> gateway PUT/GET.
 TrustPolicy trust;
 std::vector<Key> auditors;
 for(int i=0;i<5;i++) {
  auditors.emplace_back(keygen());
  trust.pinned_auditors.push_back(public_key(auditors.back().get()));
 }
 MemberTicket ticket;
 ticket.node_identity=public_key(node.get());
 ticket.ticket_nonce[0]=15;
 ticket.offered_bytes=minimum_bytes;
 ticket.audited_bytes=minimum_bytes;
 ticket.issued_at=now-10;ticket.expires_at=now+300;
 for(int i=0;i<3;i++)
  ticket.auditors.push_back({public_key(auditors[i].get()),
                            sign_bytes(auditors[i].get(),audit_transcript(ticket))});
 Challenge challenge{},exporter{};
 challenge[0]=6;exporter[0]=8;
 auto proof=sign_bytes(node.get(),session_transcript(ticket,challenge,exporter));
 AuthenticatedPeer member,guest;
 assert(authenticate_session(ticket,trust,challenge,exporter,proof,now,member));
 const auto root=fs::temp_directory_path()/("meshcloud-published-test-"+
  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 {
  exchange::SharedShardStore store_a(root/"a",1024),store_b(root/"b",1024);
  assert(store_a.open()&&store_b.open());
  exchange::PeerGateway A(store_a),B(store_b);
  std::vector<std::uint8_t> frame,reply,block;
  assert(exchange::make_put(refs[0].cid,block_a,frame));
  assert(!A.handle(frame,guest,challenge,exporter,now,reply)&&reply.empty());
  assert(A.handle(frame,member,challenge,exporter,now,reply));
  assert(exchange::make_put(refs[1].cid,block_b,frame));
  assert(B.handle(frame,member,challenge,exporter,now,reply));
  const exchange::PeerGateway* gateways[]={&A,&B};
  for(std::size_t i=0;i<refs.size();i++) {
   assert(exchange::make_get(refs[i].cid,frame));
   assert(gateways[i]->handle(frame,member,challenge,exporter,now,reply));
   assert(exchange::read_get_response(reply,refs[i].cid,member,
                                       challenge,exporter,now,block));
   assert(block.size()==refs[i].bytes&&cid::matches(refs[i].cid,block));
  }
  // Wrong publisher never yields a trusted chunk list, even when peer replies.
  assert(!flatten(pages,bad_pub,now,refs));
  assert(refs.empty());
 }
 fs::remove_all(root);
 std::cout<<"Signed encrypted CID pages, tamper/expiry rejection and member-only two-store restore passed\n";
}
