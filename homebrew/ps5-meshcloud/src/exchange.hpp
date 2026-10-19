// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "wire.hpp"
#include "vault.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct sqlite3;

namespace meshcloud::exchange {
// V1 exchange transports only ciphertext bytes, never plaintext, file keys or
// private local-vault metadata. CID is 59 ASCII bytes (CIDv1/raw/SHA2-256).
constexpr std::size_t cid_ascii_size=59;
constexpr std::size_t encrypted_block_limit=chunk_bytes+36;
constexpr std::uint64_t staging_ttl_seconds=90ULL*24*60*60;

// Return code is visible only AFTER a verified member handshake.
// A successful *fragment GET* must NEVER reset a 90-day FILE expiration.
enum class Result : std::uint8_t {
 Ok=0, NotFound=1, QuotaExceeded=2, Malformed=3, StorageFailure=4
};
class SharedShardStore {
public:
 SharedShardStore(std::filesystem::path directory,std::uint64_t maximum_bytes);
 ~SharedShardStore();
 SharedShardStore(const SharedShardStore&)=delete;
 SharedShardStore& operator=(const SharedShardStore&)=delete;
 bool open();
 // Removes expired/abandoned ciphertext fragments. Does not alter the private vault.
 bool prune(std::uint64_t now,std::uint64_t& deleted);
 const std::string& error() const noexcept {return error_;}
private:
 friend class PeerGateway;
 Result put(const std::string& cid,const std::vector<std::uint8_t>& encrypted,
            std::uint64_t now);
 Result get(const std::string& cid,std::uint64_t now,
            std::vector<std::uint8_t>& encrypted);
 bool sql(const char* statement);
 std::filesystem::path directory_;
 std::uint64_t max_bytes_;
 ProcessLock writer_lock_;
 sqlite3* db_=nullptr;
 std::string error_;
};
// Transport-neutral endpoint called *inside* an authenticated TLS/Noise-like
// channel. No socket listener, NAT traversal, IP discovery or network exposure.
// An AuthenticatedPeer can only originate from admission::authenticate_session.
class PeerGateway {
public:
 explicit PeerGateway(SharedShardStore& store):store_(store){}
 // Rejects unauthorized/expired/unbound peer before any disk access.
 // PUT frame payload = 59-byte CID + encrypted MCB1 chunk.
 // GET frame payload = exactly 59-byte CID.
 // GET success reply = 59-byte CID + chunk; status reply is one Result byte.
 bool handle(const std::vector<std::uint8_t>& request,
             const admission::AuthenticatedPeer& peer,
             const admission::Challenge& challenge,
             const admission::Challenge& channel_exporter,
             std::uint64_t now,std::vector<std::uint8_t>& response);
private:
 SharedShardStore& store_;
};
bool make_put(const std::string& cid,const std::vector<std::uint8_t>& ciphertext,
              std::vector<std::uint8_t>& frame);
bool make_get(const std::string& cid,std::vector<std::uint8_t>& frame);
// Verifies response CID + SHA2-256 before caller accepts the ciphertext.
bool read_get_response(const std::vector<std::uint8_t>& encoded,
     const std::string& requested_cid,const admission::AuthenticatedPeer& peer,
     const admission::Challenge& challenge,
     const admission::Challenge& exporter,std::uint64_t now,
     std::vector<std::uint8_t>& verified_ciphertext);
} // namespace meshcloud::exchange
