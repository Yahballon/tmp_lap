// SPDX-License-Identifier: GPL-3.0-or-later
#include "stream.hpp"
#include <array>
#include <fstream>
#include <limits>
#include <vector>
#include <openssl/evp.h>
namespace meshcloud::cid {
namespace {
constexpr char base32[]="abcdefghijklmnopqrstuvwxyz234567";
std::string digest_cid(const unsigned char* digest) {
 std::array<unsigned char,36> raw{};
 raw[0]=1;raw[1]=0x55;raw[2]=0x12;raw[3]=32;
 for(std::size_t i=0;i<32;i++)raw[i+4]=digest[i];
 std::string out="b";
 unsigned accumulator=0,bits=0;
 for(const auto b:raw) {
  accumulator=(accumulator<<8)|b;bits+=8;
  while(bits>=5) {bits-=5;out+=base32[(accumulator>>bits)&31];}
  accumulator&=(1U<<bits)-1;
 }
 if(bits)out+=base32[(accumulator<<(5-bits))&31];
 return out;
}
}
bool from_stream(std::istream& input,std::string& result,std::uint64_t& bytes) {
 result.clear();bytes=0;
 EVP_MD_CTX* context=EVP_MD_CTX_new();
 if(!context)return false;
 bool ok=EVP_DigestInit_ex(context,EVP_sha256(),nullptr)==1;
 std::vector<char> buffer(1024*1024);
 while(ok) {
  input.read(buffer.data(),static_cast<std::streamsize>(buffer.size()));
  const auto count=input.gcount();
  if(count<0 || (count==0 && input.bad())) {ok=false;break;}
  if(count>0) {
   if(static_cast<std::uint64_t>(count)>std::numeric_limits<std::uint64_t>::max()-bytes) {
    ok=false;break;
   }
   bytes+=static_cast<std::uint64_t>(count);
   ok=EVP_DigestUpdate(context,buffer.data(),static_cast<std::size_t>(count))==1;
  }
  if(input.bad() || (input.fail() && !input.eof())) {ok=false;break;}
  if(input.eof())break;
  if(count==0) {ok=false;break;}
 }
 unsigned char digest[32]{};unsigned n=0;
 if(ok)ok=EVP_DigestFinal_ex(context,digest,&n)==1&&n==32;
 EVP_MD_CTX_free(context);
 if(!ok){bytes=0;return false;}
 result=digest_cid(digest);
 return true;
}
bool from_file(const std::filesystem::path& path,std::string& result,std::uint64_t& bytes) {
 // Explicitly reject non-regular sources such as devices, sockets and FIFOs.
 std::error_code error;
 if(!std::filesystem::is_regular_file(path,error)||error)return false;
 std::ifstream input(path,std::ios::binary);
 if(!input)return false;
 return from_stream(input,result,bytes);
}
}
