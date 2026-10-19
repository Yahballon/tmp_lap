// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "peer_discovery.hpp"
#include <openssl/evp.h>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>
namespace fs=std::filesystem;
using namespace meshcloud::discovery;
using meshcloud::admission::PublicKey;
using Secret=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
namespace {
Secret make_key(){
 auto* ctx=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);assert(ctx);
 assert(EVP_PKEY_keygen_init(ctx)==1);
 EVP_PKEY* k=nullptr;assert(EVP_PKEY_keygen(ctx,&k)==1);
 EVP_PKEY_CTX_free(ctx);return Secret(k,EVP_PKEY_free);
}
PublicKey pub(EVP_PKEY* k){
 PublicKey p{};std::size_t n=p.size();
 assert(EVP_PKEY_get_raw_public_key(k,p.data(),&n)==1&&n==p.size());
 return p;
}
}
int main() {
 const std::uint64_t now=1800000000;
 auto key=make_key(),other=make_key();
 SignedAddress addr{};
 assert(parse_lan_ipv4("192.168.33.6",addr.ipv4));
 assert(!parse_lan_ipv4("8.8.8.8",addr.ipv4));
 assert(!parse_lan_ipv4("0.0.0.0",addr.ipv4));
 assert(!parse_lan_ipv4("224.1.1.1",addr.ipv4));
 assert(!parse_lan_ipv4("192.168.033.6",addr.ipv4));
 assert(!parse_lan_ipv4("192.168.33.256",addr.ipv4));
 assert(parse_lan_ipv4("10.10.2.8",addr.ipv4));
 assert(parse_lan_ipv4("192.168.33.6",addr.ipv4));
 addr.port=39718;addr.published=now-3;addr.expires=now+record_lifetime-3;
 assert(sign(addr,key.get()));
 assert(verify(addr,now));
 assert(!verify(addr,addr.expires));
 std::vector<std::uint8_t> encoded;
 assert(encode_record(addr,encoded)&&encoded.size()==record_bytes);
 SignedAddress decoded{};
 assert(decode_record(encoded.data(),encoded.size(),decoded));
 assert(verify(decoded,now)&&ipv4_text(decoded)=="192.168.33.6");
 auto bad=decoded;bad.ipv4[3]=7;assert(!verify(bad,now));
 bad=decoded;bad.signature[12]^=1;assert(!verify(bad,now));
 bad=decoded;bad.port=0;assert(!verify(bad,now));
 bad=decoded;bad.expires+=record_lifetime;assert(!verify(bad,now));
 bad=decoded;bad.ipv4={8,8,8,8};assert(!verify(bad,now));
 std::vector<SignedAddress> peers{decoded};
 assert(encode_gossip(peers,encoded));
 assert(decode_gossip(encoded,peers)&&peers.size()==1);
 encoded.pop_back();assert(!decode_gossip(encoded,peers));
 const auto dir=fs::temp_directory_path()/
     ("meshcloud-addrman-"+std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count()));
 {
  AddressBook book(dir);
  assert(book.open());
  assert(book.add(addr,now));
  std::vector<SignedAddress> got;
  assert(book.list(now,got)&&got.size()==1);
  assert(got[0].identity==pub(key.get()));
  assert(book.add(addr,now+1)); // duplicate does not renew expiry
  std::vector<std::uint8_t> request,reply;
  assert(encode_gossip({addr},request));
  assert(!authorized_getaddr(request,pub(other.get()),book,now,reply));
  assert(authorized_getaddr(request,pub(key.get()),book,now,reply));
  assert(decode_gossip(reply,got)&&got.size()==1);
  assert(authorized_addr_reply(reply,book,now));
  assert(!authorized_addr_reply(reply,book,addr.expires));
  assert(!book.add(addr,addr.expires));
  assert(book.list(addr.expires,got)&&got.empty());
 }
 {
  AddressBook reopened(dir);
  assert(reopened.open());
  assert(reopened.add(addr,now));
  std::vector<SignedAddress> got;
  assert(reopened.list(now,got,1)&&got.size()==1);
  assert(got[0].identity==pub(key.get()));
 }
 fs::remove_all(dir);
 std::cout<<"Signed LAN peers: expiry, spam cap, guest mismatch, tamper, restart and no public-IP leaks passed\n";
}
