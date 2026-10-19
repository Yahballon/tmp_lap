// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "p2p_tls.hpp"
#include <filesystem>
#include <string>
#include <openssl/evp.h>
namespace meshcloud::p2p {
// Loads an externally issued 3+-auditor ticket and *pinned* peer identities.
// No automatic auditors, self-issued membership, guest fallback or embedded
// credentials. All paths are local and must be protected by OS access controls.
class Config {
public:
 Config()=default;
 ~Config();
 Config(const Config&)=delete;
 Config& operator=(const Config&)=delete;
 bool load(const std::filesystem::path& config_directory,std::string& error);
 const NodeIdentity& node() const noexcept {return node_;}
private:
 EVP_PKEY* key_=nullptr;
 NodeIdentity node_{};
};
// Fixed-length binary ticket produced and signed by trusted external auditors.
// Data is untrusted until admission::verify_member() checks the quorum.
bool serialize_member_ticket(const admission::MemberTicket& ticket,
                             std::vector<std::uint8_t>& bytes);
bool parse_member_ticket(const std::vector<std::uint8_t>& bytes,
                         admission::MemberTicket& ticket);
} // namespace meshcloud::p2p
