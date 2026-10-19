// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
struct sqlite3;
namespace meshcloud {
constexpr std::uint64_t retention_seconds=90ULL*24*60*60;
struct CatalogEntry {
 std::string cid;
 std::uint64_t bytes=0,created_at=0,last_successful_download=0,expires_at=0;
 std::uint64_t completed_downloads=0;
 std::string display_name;
};
class Catalog {
public:
 explicit Catalog(std::filesystem::path path);
 ~Catalog();
 Catalog(const Catalog&)=delete;
 Catalog& operator=(const Catalog&)=delete;
 bool open();
 const std::string& error() const noexcept {return error_;}
 bool register_file(const CatalogEntry& entry,const std::vector<std::string>& block_cids);
 bool find(const std::string& cid,CatalogEntry& entry);
 bool list_live(std::uint64_t now,std::vector<CatalogEntry>& results,std::size_t max_count=1000);
 bool blocks(const std::string& cid,std::vector<std::string>& ordered);
 // Called only AFTER an authenticated, complete, hash-verified transfer.
 // Not triggered by search, partial download, or heartbeat.
 bool mark_completed_download(const std::string& cid,std::uint64_t now);
 bool expired(std::uint64_t now,std::vector<std::string>& cids);
 bool purge_expired(std::uint64_t now,std::vector<std::string>& removed);
 bool block_is_referenced(const std::string& cid,bool& referenced);
private:
 bool exec(const char* sql);
 bool begin();
 bool commit();
 void rollback();
 std::filesystem::path path_;
 std::string error_;
 sqlite3* db_=nullptr;
};
}
