// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
namespace meshcloud {
constexpr std::uint64_t minimum_bytes = 10ULL*1024*1024*1024;
constexpr std::uint64_t safety_bytes = 2ULL*1024*1024*1024;
enum class Operation { Discovery, Search, Listing, Metadata, Upload, Download, Publish, Replication, Relay };
enum class Denial { None, Identity, Quota, StorageProof, Expired, Protocol };
struct VerifiedMembership {
  std::uint64_t claimed_bytes=0, audited_bytes=0, expires_at=0;
  bool authenticated_identity=false, authenticated_proof=false;
  unsigned protocol=0;
};
struct Admission { bool allowed=false; Denial denial=Denial::Identity; };
Admission authorize(Operation op,const VerifiedMembership& member,std::uint64_t now) noexcept;
const char* denial_message(Denial) noexcept;
struct StoragePreflight {
  bool eligible=false;
  std::uint64_t free_bytes=0;
  std::string message;
};
StoragePreflight preflight(const std::filesystem::path& path,std::uint64_t offered_bytes) noexcept;
}
