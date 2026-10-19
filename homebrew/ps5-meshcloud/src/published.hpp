// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "admission.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <openssl/evp.h>

namespace meshcloud::published {
// Immutable, owner-signed pages of ciphertext-chunk CIDs. A file containing
// millions of chunks uses more signed pages; no 32 MiB or 4 GiB file limit.
// Private AES-GCM decryption keys and filenames NEVER belong in these pages.
constexpr std::size_t max_chunks_per_page=128;
constexpr std::uint64_t page_retention_seconds=90ULL*24*60*60;
struct ChunkRef {
 std::string cid;
 std::uint32_t bytes=0; // AES-GCM MCB1 encrypted chunk length including header
};
struct Page {
 admission::PublicKey publisher{};
 std::string file_manifest_cid;
 std::uint64_t created_at=0,issued_at=0,expires_at=0;
 std::uint64_t ciphertext_bytes=0; // Across ALL pages, exact total on disk
 std::uint32_t index=0,page_count=0;
 std::vector<ChunkRef> chunks;
 admission::Signature signature{};
};
// Deterministic domain-separated bytes signed by the file PUBLISHER.
// Not an auditor credential and not a membership grant.
std::vector<unsigned char> transcript(const Page& page);
// Requires the matching Ed25519 private key; signs schema and public identity.
// File author keys are NOT peer auditor keys.
bool sign(Page& page,EVP_PKEY* publisher_private_key);
// No implicit trust of supplied manifests. Caller also MUST verify that the
// pubkey is the one expected for the given file. A valid signature from an
// unknown author doesn't make a file trustworthy.
bool verify(const Page& page,const admission::PublicKey& expected_publisher,
            std::uint64_t now);
// Verifies complete, ordered page set, one owner/file, contiguous offsets,
// no missing/duplicated page and exact ciphertext size with overflow checks.
bool verify_complete(const std::vector<Page>& pages,
                     const admission::PublicKey& expected_publisher,
                     std::uint64_t now);
// Derive an ordered immutable list only after full signature validation.
bool flatten(const std::vector<Page>& pages,
             const admission::PublicKey& expected_publisher,
             std::uint64_t now,std::vector<ChunkRef>& chunks);
} // namespace meshcloud::published
