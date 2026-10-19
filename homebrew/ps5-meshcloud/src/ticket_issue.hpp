// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "admission.hpp"
#include "storage_audit.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <openssl/evp.h>

namespace meshcloud::ticket_issue {
// Independent auditors all sign ONE node-authorized ticket proposal.
// The signed proposal is cryptographically bound to the node Ed25519 key.
// It never grants access without independently audited quorum signatures.
struct Request {
 admission::MemberTicket ticket{};
 admission::Signature owner_proof{};
};
struct Partial {
 admission::PublicKey signer{};
 admission::Signature signature{};
 std::array<unsigned char,32> request_sha256{};
};
bool new_node_identity(const std::filesystem::path& path,std::string& error);
bool request(EVP_PKEY* node_key,std::uint64_t now,Request& output);
bool verify_request(const Request& request,std::uint64_t now);
bool encode_request(const Request&,std::vector<unsigned char>& out);
bool decode_request(const std::vector<unsigned char>& raw,Request& out);
bool encode_partial(const Partial&,std::vector<unsigned char>& out);
bool decode_partial(const std::vector<unsigned char>& raw,Partial& out);
bool attest(EVP_PKEY* independent_auditor,const Request& request,
    const storage_audit::Ledger& independent_commitment,
    storage_audit::VerifiedSample& remote_verified,
    std::uint64_t now,Partial& output);
// Requires minimum three cryptographically VALID different pinned auditors.
// Never accepts an auditor key from the node request or from a partial file
// as a substitute for a locally configured explicit trust policy.
bool assemble(const Request&,const std::vector<Partial>&,
    const admission::TrustPolicy& preconfigured_trust,std::uint64_t now,
    admission::MemberTicket& output);
bool verify_partial(const Request&,const Partial&);
bool load_file(const std::filesystem::path&,std::size_t max,
    std::vector<unsigned char>& out);
bool write_new(const std::filesystem::path&,const std::vector<unsigned char>&,
    std::string& error);
} // namespace meshcloud::ticket_issue
