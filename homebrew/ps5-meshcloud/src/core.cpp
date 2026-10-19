// SPDX-License-Identifier: GPL-3.0-or-later
#include "core.hpp"
#include <filesystem>
#include <limits>
namespace meshcloud {
Admission authorize(Operation operation,const VerifiedMembership& m,std::uint64_t now) noexcept {
 switch(operation) {
 case Operation::Discovery: case Operation::Search: case Operation::Listing:
 case Operation::Metadata: case Operation::Upload: case Operation::Download:
 case Operation::Publish: case Operation::Replication: case Operation::Relay: break;
 default: return {false,Denial::Protocol};
 }
 if(!m.authenticated_identity) return {false,Denial::Identity};
 if(m.protocol!=1) return {false,Denial::Protocol};
 if(m.claimed_bytes<minimum_bytes) return {false,Denial::Quota};
 if(!m.authenticated_proof || m.audited_bytes<minimum_bytes) return {false,Denial::StorageProof};
 if(m.expires_at<=now || m.expires_at-now>3600) return {false,Denial::Expired};
 return {true,Denial::None};
}
const char* denial_message(Denial why) noexcept {
 switch(why) {
 case Denial::None:return "Membership confirmed";
 case Denial::Identity:return "Authenticated identity required";
 case Denial::Quota:return "At least 10 GiB must be contributed";
 case Denial::StorageProof:return "Authenticated 10 GiB storage audit required";
 case Denial::Expired:return "Membership attestation expired";
 case Denial::Protocol:return "Protocol unsupported";
 }
 return "Access denied";
}
StoragePreflight preflight(const std::string& path,std::uint64_t offered) noexcept {
 StoragePreflight r;
 if(offered<minimum_bytes) {r.message="At least 10 GiB must be contributed";return r;}
 if(offered>std::numeric_limits<std::uint64_t>::max()-safety_bytes) {
  r.message="Invalid quota";return r;
 }
 try {
  const auto root=std::filesystem::canonical(path);
  if(!std::filesystem::is_directory(root)) {r.message="Storage directory is not a directory";return r;}
  const auto info=std::filesystem::space(root);
  if(info.available==static_cast<std::uintmax_t>(-1) ||
     info.available>std::numeric_limits<std::uint64_t>::max()) {
   r.message="Disk capacity is unknown";return r;
  }
  r.free_bytes=static_cast<std::uint64_t>(info.available);
  if(r.free_bytes<offered+safety_bytes) {
   r.message="Insufficient free disk space: quota plus 2 GiB margin required";return r;
  }
  r.eligible=true;
  r.message="Local disk preflight passed; no network admission granted";
  return r;
 } catch(...) {
  r.message="Could not verify local storage";return r;
 }
}
}
