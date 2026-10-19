// SPDX-License-Identifier: GPL-3.0-or-later
#include "p2p_config.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <openssl/pem.h>
#include <system_error>
namespace meshcloud::p2p {
namespace {
namespace fs=std::filesystem;
bool ordinary(const fs::path& p) {
 std::error_code ec;
 return fs::symlink_status(p,ec).type()==fs::file_type::regular&&!ec;
}
int hexdigit(char value) {
 if(value>='0'&&value<='9')return value-'0';
 if(value>='a'&&value<='f')return value-'a'+10;
 if(value>='A'&&value<='F')return value-'A'+10;
 return -1;
}
bool parse_hex(const std::string& value,admission::PublicKey& key) {
 if(value.size()!=key.size()*2)return false;
 for(std::size_t i=0;i<key.size();i++){
  const int a=hexdigit(value[i*2]),b=hexdigit(value[i*2+1]);
  if(a<0||b<0)return false;
  key[i]=static_cast<unsigned char>(a*16+b);
 }
 return key!=admission::PublicKey{};
}
bool parse_keys(const fs::path& path,std::vector<admission::PublicKey>& result,
                std::size_t least,std::string& error) {
 result.clear();
 if(!ordinary(path)) {error="Trust/allowlist file is missing";return false;}
 std::ifstream input(path,std::ios::binary);
 if(!input){error="Cannot read local key list";return false;}
 std::string line;
 while(std::getline(input,line)) {
  if(line.size()>128){error="Invalid public key line length";return false;}
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.empty())continue;
  admission::PublicKey key{};
  if(!parse_hex(line,key) ||
     std::find(result.begin(),result.end(),key)!=result.end() ||
     result.size()>=32) {
   error="Invalid, duplicate or excessive public key in allowlist";
   return false;
  }
  result.push_back(key);
 }
 if(!input.eof()||result.size()<least) {
  error="Insufficient trusted independent public keys";return false;
 }
 return true;
}
bool read_small(const fs::path& path,std::size_t max,std::vector<unsigned char>& buffer) {
 buffer.clear();
 if(!ordinary(path))return false;
 std::error_code ec;
 const auto length=fs::file_size(path,ec);
 if(ec||length==0||length>max)return false;
 std::ifstream in(path,std::ios::binary);
 if(!in)return false;
 buffer.resize(static_cast<std::size_t>(length));
 in.read(reinterpret_cast<char*>(buffer.data()),
         static_cast<std::streamsize>(buffer.size()));
 return in.gcount()==static_cast<std::streamsize>(buffer.size())&&!in.bad();
}
}
Config::~Config(){
 if(key_)EVP_PKEY_free(key_);
}
bool Config::load(const std::filesystem::path& root,std::string& error) {
 error.clear();
 // Unlike guest onboarding, every node must come with a pre-issued 10 GiB
 // audit ticket signed by an out-of-band quorum. Never mint one here.
 if(key_){EVP_PKEY_free(key_);key_=nullptr;}
 node_={};
 std::vector<unsigned char> secret, ticket;
 if(!read_small(root/"identity.pem",64*1024,secret) ||
    !read_small(root/"member.ticket",max_ticket_bytes,ticket)){
  error="Missing identity.pem or member.ticket; no unaudited P2P access";
  return false;
 }
 BIO* bio=BIO_new_mem_buf(secret.data(),static_cast<int>(secret.size()));
 if(!bio){error="Could not parse private key";return false;}
 EVP_PKEY* key=PEM_read_bio_PrivateKey(bio,nullptr,nullptr,nullptr);
 BIO_free(bio);
 std::fill(secret.begin(),secret.end(),0);
 if(!key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519){
  if(key)EVP_PKEY_free(key);
  error="Private key must be an Ed25519 PEM key";return false;
 }
 key_=key;
 if(!parse_member_ticket(ticket,node_.ticket) ||
    !parse_keys(root/"auditors.keys",node_.auditors.pinned_auditors,3,error) ||
    !parse_keys(root/"peers.keys",node_.allowed_peer_keys,1,error)) {
  if(error.empty())error="Invalid or expired-format audit ticket";
  EVP_PKEY_free(key_);key_=nullptr;node_={};return false;
 }
 node_.auditors.quorum=3;
 admission::PublicKey own{};std::size_t size=own.size();
 if(EVP_PKEY_get_raw_public_key(key_,own.data(),&size)!=1 ||
    size!=own.size() || own!=node_.ticket.node_identity) {
  error="Ticket identity does not match local private key";
  EVP_PKEY_free(key_);key_=nullptr;node_={};return false;
 }
 if(std::find(node_.allowed_peer_keys.begin(),node_.allowed_peer_keys.end(),own)
    !=node_.allowed_peer_keys.end()) {
  error="A node cannot whitelist its own identity as a remote peer";
  EVP_PKEY_free(key_);key_=nullptr;node_={};return false;
 }
 node_.private_key=key_;
 return true;
}
} // namespace meshcloud::p2p
