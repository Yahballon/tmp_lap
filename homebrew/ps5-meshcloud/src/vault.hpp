// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "catalog.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace meshcloud {
using FileKey=std::array<unsigned char,32>;
constexpr std::size_t chunk_bytes=4*1024*1024;
bool make_key(FileKey& key);
bool save_key(const std::filesystem::path& path,const FileKey& key);
bool load_key(const std::filesystem::path& path,FileKey& key);
class ProcessLock {
public:
 ProcessLock()=default;
 ~ProcessLock();
 ProcessLock(const ProcessLock&)=delete;
 ProcessLock& operator=(const ProcessLock&)=delete;
 bool acquire(const std::filesystem::path& path);
private:
 std::intptr_t native_=-1;
};
class LocalVault {
public:
 explicit LocalVault(std::filesystem::path directory);
 bool open();
 // Local-only offline functionality. Not P2P upload/download or proof of membership.
 bool import_file(const std::filesystem::path& input,std::uint64_t now,
                  const FileKey& key,std::string& file_cid);
 bool export_file(const std::string& cid,const std::filesystem::path& output,
                  const FileKey& key,std::uint64_t now);
 bool prune(std::uint64_t now,std::uint64_t& expired_files,std::uint64_t& deleted_blocks);
 const std::string& error() const noexcept {return error_;}
 Catalog& catalog() noexcept {return catalog_;}
private:
 std::filesystem::path root_;
 ProcessLock process_lock_;
 Catalog catalog_;
 std::string error_;
 bool write_block(const std::string& cid,const std::vector<unsigned char>& ciphertext);
 bool read_block(const std::string& cid,std::vector<unsigned char>& ciphertext);
};
}
