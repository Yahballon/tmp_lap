// SPDX-License-Identifier: GPL-3.0-or-later
// Explicit large-disk benchmark; NEVER issues network tickets locally.
#include "storage_audit.hpp"
#include "core.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <openssl/rand.h>
#include <openssl/pem.h>
namespace fs=std::filesystem;
int main(int argc,char** argv){
 if(argc==3&&std::string(argv[1])=="pubkey"){
  BIO* bio=BIO_new_file(argv[2],"r");
  if(!bio)return 1;
  EVP_PKEY* key=PEM_read_bio_PrivateKey(bio,nullptr,
     [](char*,int,int,void*)->int{return 0;},nullptr);
  BIO_free(bio);
  if(!key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519){
   if(key)EVP_PKEY_free(key);
   return 1;
  }
  meshcloud::admission::PublicKey id{};std::size_t length=id.size();
  const bool okay=EVP_PKEY_get_raw_public_key(key,id.data(),&length)==1&&length==id.size();
  EVP_PKEY_free(key);
  if(!okay)return 1;
  constexpr char digits[]="0123456789abcdef";
  for(auto b:id)std::cout<<digits[b>>4]<<digits[b&15];
  std::cout<<"\n";return 0;
 }
 const bool provision_for=argc>1&&std::string(argv[1])=="provision-for";
 if(!((argc==5&&std::string(argv[1])=="local-check")||(provision_for&&argc==6))){
  std::cout<<"MeshCloud Storage Audit v0.10 by YAHBALLON\n"
    <<"Usage: meshcloud-audit local-check <node-random-data.bin> "
    <<"<auditor-hashes.mcal> <gigabytes>\n"
    <<"   meshcloud-audit provision-for <node-public-hex> <node-data> <auditor-ledger> 13\n"
    <<"   meshcloud-audit pubkey <identity.pem>\n"
    <<"WARNING: writes real incompressible data to disk and retains it!\n"
    <<"Requires free space >= requested GiB. NO membership ticket is issued.\n"
    <<"An independent auditor is still needed for actual network admission.\n";
  return argc==2&&std::string(argv[1])=="--help"?0:2;
 }
 std::uint64_t gib=0;
 try{
  const std::string input=argv[provision_for?5:4];
  if(input.empty()||input.find_first_not_of("0123456789")!=std::string::npos)
   return 2;
  gib=std::stoull(input);
 }catch(...){return 2;}
 if(gib<1||gib>13){
  std::cerr<<"Select 1..13 GiB; network membership requires exactly 13 GiB\n";
  return 2;
 }
 const auto size=gib*1024ULL*1024*1024;
 const fs::path file=argv[provision_for?3:2],commitment=argv[provision_for?4:3];
 std::error_code ec;
 const auto data_status=fs::symlink_status(file,ec);
 const bool missing_data=data_status.type()==fs::file_type::not_found &&
    (!ec||ec==std::errc::no_such_file_or_directory);
 ec.clear();
 const auto audit_status=fs::symlink_status(commitment,ec);
 const bool missing_audit=audit_status.type()==fs::file_type::not_found &&
    (!ec||ec==std::errc::no_such_file_or_directory);
 if(!missing_data||!missing_audit) {
  std::cerr<<"Refusing to overwrite existing files\n";return 1;
 }
 const auto node_dir=file.parent_path().empty()?fs::path("."):file.parent_path();
 const auto ledger_dir=commitment.parent_path().empty()?fs::path("."):commitment.parent_path();
 if(!fs::is_directory(node_dir,ec)||ec||!fs::is_directory(ledger_dir,ec)||ec){
  std::cerr<<"Both parent directories must already exist\n";return 1;
 }
 const auto disk=fs::space(node_dir,ec);
 if(ec||disk.available<size+meshcloud::safety_bytes){
  std::cerr<<"Not enough free physical disk space for random audit data and margin\n";return 1;
 }
 meshcloud::admission::PublicKey identity{};
 if(provision_for){
  const std::string hex=argv[2];
  if(hex.size()!=64){std::cerr<<"Node public identity must be 64 hex chars\n";return 2;}
  auto nibble=[](char c)->int {
   if(c>='0'&&c<='9')return c-'0';
   if(c>='a'&&c<='f')return c-'a'+10;
   if(c>='A'&&c<='F')return c-'A'+10;
   return -1;
  };
  for(int i=0;i<32;i++) {
   const auto hi=nibble(hex[2*i]),lo=nibble(hex[2*i+1]);
   if(hi<0||lo<0){std::cerr<<"Invalid node identity hex\n";return 2;}
   identity[static_cast<std::size_t>(i)]=static_cast<unsigned char>((hi<<4)|lo);
  }
  if(identity==meshcloud::admission::PublicKey{})return 2;
  if(gib!=13){std::cerr<<"Remote-node provisioning requires exactly 13 GiB\n";return 2;}
 }else if(RAND_bytes(identity.data(),static_cast<int>(identity.size()))!=1){
  std::cerr<<"Secure random identity failed\n";return 1;
 }
 std::cout<<"Generating "<<gib<<" GiB of new random, incompressible audit data...\n";
 std::string error;
 if(!meshcloud::storage_audit::provision(file,commitment,identity,size,error)){
  std::cerr<<"Cannot provision: "<<error<<"\n";return 1;
 }
 meshcloud::storage_audit::Ledger ledger{};
 if(!meshcloud::storage_audit::load_ledger(commitment,ledger)){
  std::cerr<<"Cannot load auditor hash ledger\n";return 1;
 }
 const auto time=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
 if(time<=0)return 1;
 const auto now=static_cast<std::uint64_t>(time);
 meshcloud::storage_audit::Challenge challenge{};
 meshcloud::storage_audit::Answer answer{};
 meshcloud::storage_audit::VerifiedSample verified;
 bool consumed=false;
 if(!meshcloud::storage_audit::make_challenge(ledger,now,challenge)||
    !meshcloud::storage_audit::answer(file,challenge,answer)||
    !meshcloud::storage_audit::verify(ledger,challenge,answer,now,consumed,verified)){
  std::cerr<<"Randomized audit challenge FAILED\n";return 1;
 }
 std::cout<<"PASSED: "<<meshcloud::storage_audit::challenge_samples
   <<" independent unpredictable 64KiB samples checked from "<<gib
   <<" GiB. Disk file and auditor ledger retained at requested paths.\n"
   <<"NOT a remote audit. No signed ticket, P2P membership, or storage reservation "
   <<"is granted. Run auditors on independent machines before admitting nodes.\n";
 return 0;
}
