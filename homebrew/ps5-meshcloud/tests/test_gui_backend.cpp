// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "vault.hpp"
#include "catalog.hpp"
#include "cid.hpp"
#include "stream.hpp"
#include <sqlite3.h>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace fs=std::filesystem;
int main() {
 const auto count=std::chrono::steady_clock::now().time_since_epoch().count();
 const auto root=fs::temp_directory_path()/("meshcloud-gui-backend-"+std::to_string(count));
 fs::create_directories(root);
 const auto source=root/"example file.bin";
 const auto target=root/"restored.bin";
 {
  std::ofstream f(source,std::ios::binary);
  std::vector<char> bytes(1024*1024,'M');
  for(int i=0;i<9;i++)f.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));
 }
 fs::create_directories(root/"vault");
 // Force a backward-compatible migration from the exact older catalog schema.
 {
  sqlite3* db=nullptr;
  const auto db_path=(root/"vault"/"catalog.sqlite").string();
  assert(sqlite3_open(db_path.c_str(),&db)==SQLITE_OK);
  assert(sqlite3_exec(db,"CREATE TABLE files(cid TEXT PRIMARY KEY,"
   "bytes INTEGER NOT NULL CHECK(bytes>=0),created_at INTEGER NOT NULL,"
   "last_success INTEGER NOT NULL,expires_at INTEGER NOT NULL,"
   "complete_count INTEGER NOT NULL DEFAULT 0);",nullptr,nullptr,nullptr)==SQLITE_OK);
  assert(sqlite3_close(db)==SQLITE_OK);
 }
 const auto begin=1700000000ULL;
 {
 meshcloud::LocalVault vault(root/"vault");
 assert(vault.open());
 std::vector<meshcloud::CatalogEntry> items;
 assert(vault.catalog().list_live(begin,items));
 assert(items.empty());
 std::string aborted_cid;std::uint64_t aborted_bytes=0;
 assert(!meshcloud::cid::from_file(source,aborted_cid,aborted_bytes,
   [](std::uint64_t completed,std::uint64_t total) {
    assert(completed<=total);return false;
   }));
 assert(aborted_cid.empty()&&aborted_bytes==0);
 meshcloud::FileKey key{};
 assert(meshcloud::make_key(key));
 std::string id;
 std::uint64_t reported=0;
 bool cancelled=false;
 assert(!vault.import_file(source,begin,key,id,[&](std::uint64_t done,std::uint64_t total) {
  assert(done<=total);reported=done;cancelled=true;return false;
 }));
 assert(cancelled&&id.empty());
 assert(vault.catalog().list_live(begin,items)&&items.empty());
 std::uint64_t expired=0,unreferenced=0;
 assert(vault.prune(begin,expired,unreferenced));
 assert(unreferenced>=1);
 reported=0;
 assert(vault.import_file(source,begin,key,id,[&](std::uint64_t done,std::uint64_t total) {
  assert(done<=total);
  reported=done;return true;
 }));
 assert(reported==9ULL*1024*1024);
 assert(vault.catalog().list_live(begin,items));
 assert(items.size()==1&&items[0].cid==id);
 assert(items[0].display_name=="example file.bin");
 assert(items[0].bytes==reported);
 // Partial local restore must NOT refresh retention or leave output files.
 assert(!vault.export_file(id,target,key,begin+300,[&](std::uint64_t done,std::uint64_t total) {
  assert(done<=total);return false;
 }));
 assert(!fs::exists(target));
 meshcloud::CatalogEntry entry{};
 assert(vault.catalog().find(id,entry));
 assert(entry.completed_downloads==0);
 assert(entry.expires_at==begin+meshcloud::retention_seconds);
 assert(vault.export_file(id,target,key,begin+400));
 assert(fs::file_size(target)==reported);
 assert(vault.catalog().find(id,entry));
 assert(entry.completed_downloads==1);
 assert(entry.expires_at==begin+400+meshcloud::retention_seconds);
 assert(vault.catalog().list_live(begin+meshcloud::retention_seconds+1,items)&&items.size()==1);
 assert(vault.catalog().list_live(entry.expires_at,items)&&items.empty());
 }
 {
  const auto unicode_root=root/fs::path(std::u8string(u8"vault-äö漢字"));
  meshcloud::LocalVault unicode_vault(unicode_root);
  assert(unicode_vault.open());
  std::vector<meshcloud::CatalogEntry> unicode_items;
  assert(unicode_vault.catalog().list_live(begin,unicode_items));
  assert(unicode_items.empty());
 }
 fs::remove_all(root);
 std::cout<<"GUI backend: migrated v0.3 catalog, UTF-8 names, cancellable streaming, recovery and lifecycle passed\n";
}
