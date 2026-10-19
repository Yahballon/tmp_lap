// SPDX-License-Identifier: GPL-3.0-or-later
#include "cid.hpp"
#include <array>
#include <openssl/evp.h>
namespace meshcloud::cid {
namespace {
constexpr char alphabet[]="abcdefghijklmnopqrstuvwxyz234567";
std::string base32(const std::uint8_t* data,std::size_t n) {
 std::string out="b";unsigned buffer=0;unsigned bits=0;
 for(std::size_t i=0;i<n;i++) {
  buffer=(buffer<<8)|data[i];bits+=8;
  while(bits>=5) {bits-=5;out.push_back(alphabet[(buffer>>bits)&31]);}
  buffer&=(1U<<bits)-1;
 }
 if(bits)out.push_back(alphabet[(buffer<<(5-bits))&31]);
 return out;
}
bool decoded(const std::string& s,std::array<std::uint8_t,36>& out) {
 if(s.empty()||s[0]!='b'||s.size()!=59)return false;
 unsigned accumulator=0,bits=0;std::size_t count=0;
 for(std::size_t i=1;i<s.size();i++) {
  const auto ch=s[i];unsigned digit=32;
  for(unsigned j=0;j<32;j++)if(ch==alphabet[j]) {digit=j;break;}
  if(digit==32)return false;
  accumulator=(accumulator<<5)|digit;bits+=5;
  if(bits>=8) {
   bits-=8;
   if(count>=out.size())return false;
   out[count++]=(std::uint8_t)((accumulator>>bits)&255);
   accumulator&=(1U<<bits)-1;
  }
 }
 return count==out.size() && (bits==0||accumulator==0) &&
   out[0]==1 && out[1]==0x55 && out[2]==0x12 && out[3]==0x20;
}
}
std::string from_bytes(const std::vector<std::uint8_t>& data) {
 std::array<std::uint8_t,36> bytes{};
 bytes[0]=1;bytes[1]=0x55;bytes[2]=0x12;bytes[3]=0x20;
 EVP_MD_CTX* ctx=EVP_MD_CTX_new();if(!ctx)return {};
 unsigned len=0;
 bool ok=EVP_DigestInit_ex(ctx,EVP_sha256(),nullptr)==1 &&
   EVP_DigestUpdate(ctx,data.data(),data.size())==1 &&
   EVP_DigestFinal_ex(ctx,bytes.data()+4,&len)==1 && len==32;
 EVP_MD_CTX_free(ctx);
 return ok?base32(bytes.data(),bytes.size()):std::string();
}
bool validate(const std::string& id) {
 std::array<std::uint8_t,36> decoded_bytes{};
 if(!decoded(id,decoded_bytes))return false;
 return base32(decoded_bytes.data(),decoded_bytes.size())==id;
}
bool matches(const std::string& id,const std::vector<std::uint8_t>& bytes) {
 return validate(id) && from_bytes(bytes)==id;
}
} // namespace meshcloud::cid
