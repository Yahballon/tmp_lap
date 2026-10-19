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
namespace fs=std::filesystem;
int main(int argc,char** argv){
 if(argc!=5||std::string(argv[1])!="local-check"){
  std::cout<<"MeshCloud Storage Audit v0.9 by YAHBALLON\n"
    <<"Usage: meshcloud-audit local-check <node-random-data.bin> "
    <<"<auditor-hashes.mcal> <gigabytes>\n"
    <<"WARNING: writes real incompressible data to disk and retains it!\n"
    <<"Requires free space >= requested GiB. NO membership ticket is issued.\n"
    <<"An independent auditor is still needed for actual network admission.\n";
  return argc==2&&std::string(argv[1])=="--help"?0:2;
 }
 std::uint64_t gib=0;
 try{
  const std::string input=argv[4];
  if(input.empty()||input.find_first_not_of("0123456789")!=std::string::npos)
   return 2;
  gib=std::stoull(input);
 }catch(...){return 2;}
 if(gib<1||gib>1024){
  std::cerr<<"Select 1..1024 GiB; true membership requires >= 10 GiB\n";
  return 2;
 }
 const auto size=gib*1024ULL*1024*1024;
 const fs::path file=argv[2],commitment=argv[3];
 std::error_code ec;
 if(fs::exists(file,ec)||ec||fs::exists(commitment,ec)||ec) {
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
 if(RAND_bytes(identity.data(),static_cast<int>(identity.size()))!=1){
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
