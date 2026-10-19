// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "erasure.hpp"
#include "cid.hpp"
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>
using meshcloud::erasure::Bytes;
using meshcloud::erasure::Codec;
namespace {
void scenario(std::size_t k,std::size_t parity,std::size_t loss,
              std::uint32_t seed) {
 const Codec codec(k,parity);
 assert(codec.valid());
 std::mt19937 rng(seed);
 std::vector<Bytes> raw(k,Bytes(1024));
 for(auto& stripe:raw)
  for(auto& b:stripe)b=static_cast<std::uint8_t>(rng()%256);
 std::vector<Bytes> encoded;
 assert(codec.encode(raw,encoded));
 assert(encoded.size()==k+parity);
 for(std::size_t i=0;i<k;i++)assert(encoded[i]==raw[i]);
 std::vector<std::string> identities;
 for(const auto& fragment:encoded)identities.push_back(meshcloud::cid::from_bytes(fragment));
 for(unsigned shuffle=0;shuffle<4;shuffle++) {
  std::vector<std::optional<Bytes>> available(encoded.begin(),encoded.end());
  std::vector<std::size_t> indices(encoded.size());
  for(std::size_t i=0;i<indices.size();i++)indices[i]=i;
  std::shuffle(indices.begin(),indices.end(),rng);
  for(std::size_t i=0;i<loss;i++)available[indices[i]]=std::nullopt;
  std::vector<Bytes> recovered;
  assert(codec.recover(available,identities,recovered));
  assert(recovered==raw);
 }
 // Losing more than the parity budget must fail.
 std::vector<std::optional<Bytes>> missing(encoded.begin(),encoded.end());
 for(std::size_t i=0;i<parity+1;i++)missing[i]=std::nullopt;
 std::vector<Bytes> reconstructed;
 assert(!codec.recover(missing,identities,reconstructed));
 // Poisoned bytes are discarded rather than counted as valid shards.
 auto partial=std::vector<std::optional<Bytes>>(encoded.begin(),encoded.end());
 for(std::size_t i=0;i<parity;i++)partial[i]=std::nullopt;
 auto present=parity;
 (*partial[present])[0]^=0x80;
 assert(!codec.recover(partial,identities,reconstructed));
 // The source ID list itself is expected to come from a signed manifest.
 auto invalid=identities;invalid[0]="../etc/passwd";
 assert(!codec.recover(partial,invalid,reconstructed));
}
}
int main() {
 assert(!Codec(0,20).valid());
 assert(!Codec(33,10).valid());
 assert(!Codec(10,246).valid());
 assert(Codec(10,100).valid());
 std::vector<Bytes> fragments;
 assert(!Codec(10,20).encode(std::vector<Bytes>(10),fragments));
 scenario(10,20,20,87210);
 scenario(10,100,100,95614);
 scenario(5,5,5,1177);
 std::cout<<"Reed-Solomon 10+20 and 10+100 recovery, CID tamper detection and fail-closed cases passed\n";
}
