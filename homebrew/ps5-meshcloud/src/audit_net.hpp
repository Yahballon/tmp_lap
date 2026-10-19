// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "storage_audit.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace meshcloud::audit_net {
// Dedicated "pre-membership" audit-only TLS1.3 link. This never connects to
// the file-exchange, CID discovery, replication, or member ticket listener.
// A known independent auditor is identified by its pre-pinned Ed25519 key.
// Connection is restricted to one random-block challenge, max ~3.1 MiB reply.
constexpr std::uint16_t port=39719;
bool serve_one(const std::string& bind_ip,std::uint16_t tcp_port,
  EVP_PKEY* node_private,
  const std::vector<admission::PublicKey>& pinned_auditors,
  const std::filesystem::path& audit_data,
  const storage_audit::Ledger& local_dataset,
  std::string& error);
bool challenge_once(const std::string& node_ip,std::uint16_t tcp_port,
  EVP_PKEY* independent_auditor_private,
  const admission::PublicKey& pinned_node,
  const storage_audit::Ledger& independent_ledger,
  storage_audit::VerifiedSample& verified,
  std::string& error);
} // namespace meshcloud::audit_net
