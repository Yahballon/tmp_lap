// SPDX-License-Identifier: GPL-3.0-or-later
#include "audit_net.hpp"
#include "storage_audit.hpp"
#include <openssl/pem.h>
#include <memory>
#include <string>
#include <iostream>
namespace {
using Key=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
Key private_ed25519(const char* path){
 BIO* bio=BIO_new_file(path,"r");if(!bio)return {nullptr,EVP_PKEY_free};
 EVP_PKEY* key=PEM_read_bio_PrivateKey(bio,nullptr,
    [](char*,int,int,void*)->int{return 0;},nullptr);
 BIO_free(bio);
 if(key&&EVP_PKEY_base_id(key)==EVP_PKEY_ED25519)return {key,EVP_PKEY_free};
 if(key)EVP_PKEY_free(key);
 return {nullptr,EVP_PKEY_free};
}
int hexdigit(char c){
 if(c>='0'&&c<='9')return c-'0';
 if(c>='a'&&c<='f')return c-'a'+10;
 if(c>='A'&&c<='F')return c-'A'+10;
 return -1;
}
bool parse_key(const std::string& hex,meshcloud::admission::PublicKey& out){
 if(hex.size()!=64)return false;
 for(std::size_t i=0;i<32;i++) {
  int a=hexdigit(hex[2*i]),b=hexdigit(hex[2*i+1]);
  if(a<0||b<0)return false;
  out[i]=static_cast<unsigned char>((a<<4)|b);
 }
 return out!=meshcloud::admission::PublicKey{};
}
}
int main(int argc,char** argv){
 if(argc<2){
  std::cerr<<"MeshCloud Audit Network v0.10 by YAHBALLON\n"
  <<"serve <node-identity.pem> <auditor-pub-hex> <audit-data> <ledger.mcal> <bind-IP>\n"
  <<"check <auditor-identity.pem> <independent-ledger.mcal> <node-IP>\n"
  <<"Uses pinned TLS1.3 port 39719; NO CID access, storage tickets or guest mode.\n";
  return 2;
 }
 const std::string op=argv[1];
 if((op=="serve"&&argc!=7)||(op=="check"&&argc!=5))return 2;
 if(op!="serve"&&op!="check")return 2;
 auto key=private_ed25519(argv[2]);
 if(!key){std::cerr<<"Invalid or encrypted Ed25519 PEM identity\n";return 1;}
 meshcloud::storage_audit::Ledger ledger{};
 const auto ledger_path=op=="serve"?argv[5]:argv[3];
 if(!meshcloud::storage_audit::load_ledger(ledger_path,ledger)){
  std::cerr<<"Missing or malformed independent audit ledger\n";return 1;
 }
 std::string error;
 if(op=="serve"){
  meshcloud::admission::PublicKey auditor{};
  if(!parse_key(argv[3],auditor)){std::cerr<<"Invalid pinned auditor public key\n";return 1;}
  const bool result=meshcloud::audit_net::serve_one(argv[6],meshcloud::audit_net::port,
    key.get(),{auditor},argv[4],ledger,error);
  if(!result){std::cerr<<"Audit responder denied: "<<error<<"\n";return 1;}
  std::cout<<"Authenticated audit-only TLS block challenge answered\n";
 }else{
  meshcloud::storage_audit::VerifiedSample verified;
  const bool result=meshcloud::audit_net::challenge_once(argv[4],meshcloud::audit_net::port,
   key.get(),ledger.node,ledger,verified,error);
  if(!result){std::cerr<<"Independent remote proof denied: "<<error<<"\n";return 1;}
  std::cout<<"PASS: 48 CSPRNG-selected SHA256 samples matched independent ledger over pinned TLS 1.3.\n"
            <<"Actual reserved size "<<ledger.bytes<<" bytes. NO membership ticket was granted.\n"
            <<"Three independent auditors are required to ISSUE a live ticket.\n";
 }
 return 0;
}
