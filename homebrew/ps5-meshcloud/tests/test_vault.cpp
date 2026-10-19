// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "vault.hpp"
#include "stream.hpp"
#include "cid.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace fs=std::filesystem;
int main() {
 const auto seed=std::chrono::steady_clock::now().time_since_epoch().count();
 const auto root=fs::temp_directory_path()/("meshcloud-test-"+std::to_string(seed));
 fs::create_directories(root);
 const auto input=root/"big.bin",output=root/"restored.bin";
 const auto keypath=root/"secret.key";
 {
  std::ofstream f(input,std::ios::binary);
  assert(f);
  std::vector<char> chunk(1024*1024);
  for(std::size_t j=0;j<chunk.size();j++)chunk[j]=static_cast<char>(j%251);
  for(int i=0;i<35;i++)f.write(chunk.data(),static_cast<std::streamsize>(chunk.size()));
  f.flush();assert(f.good());
 }
 std::string rawcid;std::uint64_t bytes=0;
 assert(meshcloud::cid::from_file(input,rawcid,bytes));
 assert(meshcloud::cid::validate(rawcid)&&bytes==35ULL*1024*1024);
 meshcloud::FileKey key{},wrong{};
 assert(meshcloud::make_key(key)&&meshcloud::make_key(wrong));
 assert(meshcloud::save_key(keypath,key));
 meshcloud::FileKey loaded{};assert(meshcloud::load_key(keypath,loaded)&&loaded==key);
 assert(!meshcloud::save_key(keypath,key)); // refuse overwrite existing key
 const std::uint64_t start=1700000000ULL;
 const auto renewal=start+meshcloud::retention_seconds-20;
 const auto renewed_expiry=renewal+meshcloud::retention_seconds;
 {
  meshcloud::LocalVault v(root/"vault");
  assert(v.open());
  std::string id;assert(v.import_file(input,start,loaded,id));
  assert(meshcloud::cid::validate(id));
  meshcloud::CatalogEntry entry{};
  assert(v.catalog().find(id,entry));
  assert(entry.bytes==bytes);
  assert(entry.expires_at==start+meshcloud::retention_seconds);
  assert(entry.completed_downloads==0);
  std::vector<std::string> blocks;
  assert(v.catalog().blocks(id,blocks));
  assert(blocks.size()==9); // 4MiB blocks; 35 MiB file
  for(const auto& b:blocks)assert(meshcloud::cid::validate(b));
  assert(!v.export_file(id,output,wrong,renewal));
  assert(!fs::exists(output));
  assert(v.catalog().find(id,entry));
  assert(entry.expires_at==start+meshcloud::retention_seconds);
  assert(entry.completed_downloads==0);
  assert(v.export_file(id,output,loaded,renewal));
  std::string restored_cid;std::uint64_t restored_bytes=0;
  assert(meshcloud::cid::from_file(output,restored_cid,restored_bytes));
  assert(restored_cid==rawcid&&restored_bytes==bytes);
  assert(v.catalog().find(id,entry)&&entry.completed_downloads==1);
  assert(entry.last_successful_download==renewal&&entry.expires_at==renewed_expiry);
  assert(!v.catalog().mark_completed_download(id,renewal-1));
  std::uint64_t files=99,removed=99;
  assert(v.prune(start+meshcloud::retention_seconds+1,files,removed));
  assert(files==0&&removed==0);
  assert(v.catalog().find(id,entry));
  assert(!v.export_file(id,root/"too-late.bin",loaded,renewed_expiry));
  assert(!fs::exists(root/"too-late.bin"));
  assert(v.prune(renewed_expiry,files,removed));
  assert(files==1&&removed==blocks.size());
  assert(!v.catalog().find(id,entry));
  for(const auto& b:blocks)assert(!fs::exists(root/"vault"/"blocks"/(b+".mcb")));
 }
 assert(fs::exists(output)); // downloaded user copies are not deleted by vault GC
 fs::remove_all(root);
 std::cout<<"35 MiB streaming roundtrip, key isolation, failure semantics, 90-day expiry and physical GC passed\n";
}
