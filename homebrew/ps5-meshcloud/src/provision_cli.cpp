// SPDX-License-Identifier: GPL-3.0-or-later
#include "provision.hpp"
#include "core.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <openssl/pem.h>
namespace {
using Key=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
Key private_key(const std::string& path){
 BIO* fp=BIO_new_file(path.c_str(),"r");
 if(!fp)return {nullptr,EVP_PKEY_free};
 EVP_PKEY* key=PEM_read_bio_PrivateKey(fp,nullptr,
   [](char*,int,int,void*)->int{return 0;},nullptr);
 BIO_free(fp);
 if(!key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519){
  if(key)EVP_PKEY_free(key);
  return {nullptr,EVP_PKEY_free};
 }
 return {key,EVP_PKEY_free};
}
int digit(char c){
 if(c>='0'&&c<='9')return c-'0';
 if(c>='a'&&c<='f')return c-'a'+10;
 if(c>='A'&&c<='F')return c-'A'+10;
 return -1;
}
bool parse_key(const std::string& hex,meshcloud::admission::PublicKey& key){
 key={};
 if(hex.size()!=64)return false;
 for(std::size_t i=0;i<32;i++){
  const int a=digit(hex[2*i]),b=digit(hex[2*i+1]);
  if(a<0||b<0)return false;
  key[i]=static_cast<unsigned char>((a<<4)|b);
 }
 return key!=meshcloud::admission::PublicKey{};
}
bool load_pins(const std::string& path,
               std::vector<meshcloud::admission::PublicKey>& pins){
 pins.clear();std::ifstream in(path,std::ios::binary);
 if(!in)return false;
 std::string line;
 while(std::getline(in,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.empty())continue;
  meshcloud::admission::PublicKey key{};
  if(pins.size()>=32||!parse_key(line,key)||
     std::find(pins.begin(),pins.end(),key)!=pins.end())return false;
  pins.push_back(key);
 }
 return in.eof()&&pins.size()>=3;
}
}
int main(int argc,char** argv){
 if(argc<2){
  std::cout<<"MeshCloud Provision v0.12 by YAHBALLON\n"
   <<"recv <node-identity.pem> <trusted-auditors.keys> <bind-IPv4> <data-output> <ledger-output>\n"
   <<"send <auditor-identity.pem> <node-pubkey-hex> <node-IPv4> <13GiB-source> <auditor-ledger.mcal>\n"
   <<"Only pre-pinned Ed25519 auditors and the exact target node can transfer; TCP 39720.\n"
   <<"PRODUCTION: exactly 13 GiB. No guest access, CIDs, filename browsing, or member.ticket issued.\n";
  return 2;
 }
 if(argc!=7)return 2;
 const std::string mode=argv[1];
 auto key=private_key(argv[2]);
 if(!key){std::cerr<<"Valid private Ed25519 PEM key required\n";return 1;}
 std::string error;
 if(mode=="recv"){
  std::vector<meshcloud::admission::PublicKey> pinned;
  if(!load_pins(argv[3],pinned)){
   std::cerr<<"Require at least 3 distinct pinned auditors in keys file\n";return 1;
  }
  if(!meshcloud::provision::receive_once(argv[4],meshcloud::provision::port,key.get(),
        pinned,argv[5],argv[6],meshcloud::minimum_bytes,error)){
   std::cerr<<"Provision DENIED: "<<error<<"\n";return 1;
  }
  std::cout<<"PASS: 13 GiB streamed, SHA256 verified block-by-block, fsynced and committed.\n";
  return 0;
 }
 if(mode=="send"){
  meshcloud::admission::PublicKey destination{};
  if(!parse_key(argv[3],destination)){
   std::cerr<<"Invalid exact recipient Ed25519 fingerprint\n";return 1;
  }
  if(!meshcloud::provision::send_once(argv[4],meshcloud::provision::port,key.get(),
        destination,argv[5],argv[6],meshcloud::minimum_bytes,error)){
   std::cerr<<"Provision DENIED: "<<error<<"\n";return 1;
  }
  std::cout<<"PASS: node acknowledged 13 GiB of checked audit data and independent ledger.\n";
  return 0;
 }
 std::cerr<<"Unknown provisioning operation\n";
 return 2;
}
