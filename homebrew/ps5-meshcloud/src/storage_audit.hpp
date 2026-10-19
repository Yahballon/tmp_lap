// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "admission.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <openssl/evp.h>
namespace meshcloud::storage_audit {
constexpr std::uint32_t block_bytes=65536;
constexpr std::size_t challenge_samples=48;
constexpr std::uint64_t challenge_lifetime=60;
using Digest=std::array<unsigned char,32>;
using Nonce=std::array<unsigned char,16>;
struct Ledger {
 admission::PublicKey node{};
 Nonce dataset_id{};
 std::uint64_t bytes=0;
 std::vector<Digest> block_hashes;
};
struct Challenge {
 admission::PublicKey node{};
 Nonce dataset_id{};
 Nonce challenge_id{};
 std::uint64_t deadline=0;
 std::vector<std::uint32_t> indices;
};
struct Answer {
 Nonce challenge_id{};
 std::vector<std::vector<unsigned char>> sampled_bytes;
};
// Opaque result minted ONLY after successful randomized, time-limited
// challenge verification. Caller cannot set an "audit passed" boolean.
class VerifiedSample {
public:
 VerifiedSample()=default;
 VerifiedSample(const VerifiedSample&)=delete;
 VerifiedSample& operator=(const VerifiedSample&)=delete;
private:
 friend bool verify(const Ledger&,const Challenge&,const Answer&,std::uint64_t,
                    bool&,VerifiedSample&);
 friend bool sign_ticket_after_proof(EVP_PKEY*,const Ledger&,
                                     const admission::MemberTicket&,
                                     VerifiedSample&,
                                     admission::AuditorSignature&);
 admission::PublicKey node_{};
 Nonce dataset_id_{};
 std::uint64_t bytes_=0,verified_at_=0;
 bool accepted_=false;
};
// An independent AUDITOR generates unpredictable, incompressible raw data
// and provisions it to a physically separate node. No deterministic seed.
// 10 GiB produces ~5 MiB of SHA-256 ledger without retaining random bytes.
// This is transport-neutral: independent deployment remains a separate task.
bool provision(const std::filesystem::path& node_data,
               const std::filesystem::path& auditor_ledger,
               const admission::PublicKey& node,std::uint64_t bytes,
               std::string& error);
bool load_ledger(const std::filesystem::path& path,Ledger& out);
bool make_challenge(const Ledger&,std::uint64_t now,Challenge& out);
bool answer(const std::filesystem::path& data,const Challenge&,Answer& out);
// Single-use receipt. An incorrect or repeated response is never accepted.
// Sample verification is probabilistic, not proof of exclusive physical HDD.
bool verify(const Ledger&,const Challenge&,const Answer&,std::uint64_t now,
            bool& consumed,VerifiedSample& verified);
// Only a successful >10 GiB proof can authorize an external auditor signature.
// Auditor private keys MUST NOT run on the member whose disk they inspect.
bool sign_ticket_after_proof(EVP_PKEY* independent_auditor_key,
                            const Ledger&,const admission::MemberTicket& ticket,
                            VerifiedSample& verified_proof,
                            admission::AuditorSignature& signed_audit);
} // namespace meshcloud::storage_audit
