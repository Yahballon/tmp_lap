// SPDX-License-Identifier: MIT
#pragma once
// Shared canonical PS5Coin prototype job encoding. C++17, no JSON dependency.
// This is NOT an installed PS5 miner and MUST NOT be used for real balances.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace ps5coin {
constexpr std::size_t kPrefixSize=84;
constexpr std::size_t kHeaderSize=92;
constexpr std::size_t kDigestSize=32;

inline int hex_digit(char c) {
  if(c>='0'&&c<='9')return c-'0';
  if(c>='a'&&c<='f')return c-'a'+10;
  if(c>='A'&&c<='F')return c-'A'+10;
  return -1;
}
inline bool parse_hex(const std::string& s,std::uint8_t* out,std::size_t size){
  if(s.size()!=size*2||!out)return false;
  for(std::size_t i=0;i<size;i++){
    int hi=hex_digit(s[2*i]),lo=hex_digit(s[2*i+1]);
    if(hi<0||lo<0)return false;
    out[i]=static_cast<std::uint8_t>((hi<<4)|lo);
  }
  return true;
}
inline void make_header(const std::array<std::uint8_t,kPrefixSize>& prefix,
                        std::uint64_t nonce,
                        std::array<std::uint8_t,kHeaderSize>& out){
  std::memcpy(out.data(),prefix.data(),kPrefixSize);
  for(unsigned i=0;i<8;++i)
    out[kPrefixSize+i]=static_cast<std::uint8_t>(nonce>>(i*8));
}
// Big-endian unsigned 256-bit comparison for this *prototype's* target rule.
// Do not silently replace with Monero's or any mainnet's consensus rule.
inline bool meets_target(const std::uint8_t hash[kDigestSize],
                         const std::array<std::uint8_t,kDigestSize>& target){
  for(std::size_t i=0;i<kDigestSize;++i){
    if(hash[i]<target[i])return true;
    if(hash[i]>target[i])return false;
  }
  return true;
}
} // namespace ps5coin
