// SPDX-License-Identifier: GPL-3.0-or-later
#include "core.hpp"
#include "cid.hpp"
#include "stream.hpp"
#include "vault.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
namespace {
std::uint64_t utc_now() {
 return static_cast<std::uint64_t>(
  std::chrono::duration_cast<std::chrono::seconds>(
   std::chrono::system_clock::now().time_since_epoch()).count());
}
void help() {
 std::cout<<"MeshCloud by YAHBALLON v0.8.0 pre-alpha\n"
  <<"Network disabled until independently operated storage auditors and real authenticated P2P transport are implemented.\nReed-Solomon 10+20 / 10+100, 3-of-5 ticket verification and bounded wire codec are separately tested.\n\n"
  <<"Commands:\n"
  <<"  meshcloud --storage PATH [--quota-gib 10]       Check local disk eligibility\n"
  <<"  meshcloud --cid-file FILE                      Stream CIDv1 hash; no hard file size limit\n"
  <<"  meshcloud --storage VAULT --local-import FILE --key-out KEYFILE\n"
  <<"                                                Encrypt and store an offline file; prints manifest CID\n"
  <<"  meshcloud --storage VAULT --local-export CID --key-file KEYFILE --out FILE\n"
  <<"                                                Restore and authenticate full offline file; renew TTL\n"
  <<"  meshcloud --storage VAULT --prune              Expire records after 90 days of inactivity and delete unused blocks\n"
  <<"  meshcloud --storage VAULT --info CID           Show local retention metadata\n"
  <<"  meshcloud --help                               Show this message\n"
  <<"\nALL commands above are offline/local diagnostics. No P2P uploads or downloads are enabled.\n";
}
}
int main(int argc,char** argv) {
 std::string directory=".",quota_input="10",cid_file,local_import,local_export;
 std::string key_out,key_file,out_file,info_id;
 bool prune=false;
 for(int i=1;i<argc;i++) {
  const std::string arg=argv[i];
  if(arg=="--help"||arg=="-h"){help();return 0;}
  if(arg=="--prune"){prune=true;continue;}
  if(i+1>=argc){std::cerr<<"Option missing value: "<<arg<<"\n";return 2;}
  const std::string value=argv[++i];
  if(arg=="--storage")directory=value;
  else if(arg=="--quota-gib")quota_input=value;
  else if(arg=="--cid-file")cid_file=value;
  else if(arg=="--local-import")local_import=value;
  else if(arg=="--local-export")local_export=value;
  else if(arg=="--key-out")key_out=value;
  else if(arg=="--key-file")key_file=value;
  else if(arg=="--out")out_file=value;
  else if(arg=="--info")info_id=value;
  else {std::cerr<<"Unknown option: "<<arg<<"\n";return 2;}
 }
 if(quota_input.empty()||quota_input.find_first_not_of("0123456789")!=std::string::npos) {
  std::cerr<<"Quota must be an integer (GiB)\n";return 2;
 }
 std::uint64_t quota=0;
 try {quota=std::stoull(quota_input);}catch(...) {std::cerr<<"Invalid quota\n";return 2;}
 if(quota>1000000) {std::cerr<<"Quota out of range\n";return 2;}
 if(!cid_file.empty()) {
  std::string cid;std::uint64_t n=0;
  if(!meshcloud::cid::from_file(cid_file,cid,n)) {
   std::cerr<<"Failed reading file for CID\n";return 1;
  }
  std::cout<<cid<<" ("<<n<<" bytes)\n";return 0;
 }
 int commands=(!local_import.empty())+(!local_export.empty())+prune+(!info_id.empty());
 if(commands>1){std::cerr<<"Choose one operation\n";return 2;}
 if(commands==0) {
  auto test=meshcloud::preflight(directory,quota*1024ULL*1024*1024);
  std::cout<<"MeshCloud v0.8.0 | Minimum 10 GiB | Offered "<<quota<<" GiB\n"
   <<"Local disk check: "<<(test.eligible?"PASS":"DENIED")<<" - "<<test.message<<"\n"
   <<"Network membership: DENIED (independent storage audits not yet implemented)\n"
   <<"Search, CID lookup, Upload, Download: DISABLED\n";
  return test.eligible?0:1;
 }
 meshcloud::LocalVault vault(directory);
 if(!vault.open()){std::cerr<<vault.error()<<"\n";return 1;}
 const auto now=utc_now();
 std::uint64_t expired=0,shards=0;
 // Runs automatically on every local vault operation, not just explicit --prune.
 if(!vault.prune(now,expired,shards)) {
  std::cerr<<"GC: "<<vault.error()<<"\n";return 1;
 }
 if(prune) {
  std::cout<<"Expired files: "<<expired<<", reclaimed blocks: "<<shards<<"\n";
  return 0;
 }
 if(!info_id.empty()) {
  meshcloud::CatalogEntry entry{};
  if(!vault.catalog().find(info_id,entry)) {std::cerr<<"Unknown or expired CID\n";return 1;}
  std::cout<<"CID: "<<entry.cid<<"\nSize: "<<entry.bytes
   <<"\nLast completed restore (UTC epoch): "<<entry.last_successful_download
   <<"\nExpires (UTC epoch): "<<entry.expires_at
   <<"\nCompleted restores: "<<entry.completed_downloads<<"\n";
  return 0;
 }
 if(!local_import.empty()) {
  if(key_out.empty()){std::cerr<<"--key-out required; do not lose your decryption key\n";return 2;}
  meshcloud::FileKey key{};
  if(!meshcloud::make_key(key) || !meshcloud::save_key(key_out,key)) {
   std::cerr<<"Cannot securely generate/save a new key file (will not overwrite)\n";return 1;
  }
  std::string id;
  if(!vault.import_file(local_import,now,key,id)) {
   std::cerr<<"Local import failed: "<<vault.error()<<"\n";return 1;
  }
  std::cout<<"Local encrypted import complete\nManifest CID: "<<id
   <<"\nKey file: "<<key_out<<"\nExpires 90 days after last fully completed restore\n";
  return 0;
 }
 if(!local_export.empty()) {
  if(key_file.empty()||out_file.empty()) {
   std::cerr<<"--key-file and --out required\n";return 2;
  }
  meshcloud::FileKey key{};
  if(!meshcloud::load_key(key_file,key)) {
   std::cerr<<"Invalid key file\n";return 1;
  }
  if(!vault.export_file(local_export,out_file,key,now)) {
   std::cerr<<"Local restore failed: "<<vault.error()<<"\n";return 1;
  }
  std::cout<<"Verified complete local restore; 90-day timer renewed\n";return 0;
 }
 return 2;
}
