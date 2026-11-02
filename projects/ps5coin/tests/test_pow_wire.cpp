// SPDX-License-Identifier: MIT
#include "../miner-ps5/pow_wire.hpp"
#include <cassert>
#include <cstdio>
int main() {
  std::array<std::uint8_t,ps5coin::kPrefixSize> prefix{};
  for(std::size_t i=0;i<prefix.size();i++)prefix[i]=static_cast<std::uint8_t>(i);
  std::array<std::uint8_t,ps5coin::kHeaderSize> out{};
  ps5coin::make_header(prefix,0x0102030405060708ULL,out);
  for(std::size_t i=0;i<ps5coin::kPrefixSize;i++)assert(out[i]==i);
  const std::uint8_t nonce_expected[8]={8,7,6,5,4,3,2,1};
  for(int i=0;i<8;i++)assert(out[84+i]==nonce_expected[i]);

  std::array<std::uint8_t,32> target{};
  std::uint8_t hash[32]{};
  assert(ps5coin::meets_target(hash,target));
  hash[31]=1;assert(!ps5coin::meets_target(hash,target));
  target.fill(0xff);assert(ps5coin::meets_target(hash,target));
  std::array<std::uint8_t,32> parsed{};
  assert(ps5coin::parse_hex(std::string(63,'0')+"f",parsed.data(),32));
  assert(parsed[31]==0x0f);
  assert(!ps5coin::parse_hex("zz",parsed.data(),32));
  std::puts("PASS: C++17 PS5 mining header / nonce / 256-bit target interop");
}
