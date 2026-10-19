// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "storage_audit.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <openssl/evp.h>
namespace meshcloud::provision {
// Separate from both file P2P (39718) and proof-only auditor TLS (39719).
// Provisioning is NEVER an authorization to access file/CID operations.
constexpr std::uint16_t port=39720;
// Production clients set exact_bytes = meshcloud::minimum_bytes (13 GiB).
// Smaller fixtures are supported by the transport for CI only and NEVER
// allow obtaining a member.ticket or an authenticated P2P session.
bool receive_once(const std::string& bind_ipv4,std::uint16_t tcp_port,
  EVP_PKEY* recipient_private,
  const std::vector<admission::PublicKey>& allowed_auditors,
  const std::filesystem::path& destination_data,
  const std::filesystem::path& destination_ledger,
  std::uint64_t exact_bytes,std::string& error);
bool send_once(const std::string& recipient_ipv4,std::uint16_t tcp_port,
  EVP_PKEY* independent_auditor_private,
  const admission::PublicKey& pinned_recipient,
  const std::filesystem::path& independent_data,
  const std::filesystem::path& independent_ledger,
  std::uint64_t exact_bytes,std::string& error);
} // namespace meshcloud::provision
