// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace meshcloud::erasure {
using Bytes=std::vector<std::uint8_t>;
// Systematic MDS Reed-Solomon in GF(2^8), Cauchy parity matrix.
// Encoded storage stripes have k original and m redundant independently
// placeable fragments; any k VERIFIED fragments can restore the stripe.
// Cryptographic authenticity comes from the trusted shard-CID manifest,
// NOT Reed-Solomon. 16-bit/64-bit file offsets belong to the outer format.
class Codec {
public:
 Codec(std::size_t data_shards,std::size_t parity_shards);
 bool valid() const noexcept;
 std::size_t data_shards() const noexcept {return k_;}
 std::size_t parity_shards() const noexcept {return m_;}
 std::size_t shard_count() const noexcept {return k_+m_;}
 // One stripe is bounded to at most 255 x 64 KiB in RAM. Process large
 // files as consecutive stripes; no per-file byte size ceiling here.
 static constexpr std::size_t max_shard_bytes=64*1024;
 bool encode(const std::vector<Bytes>& data,std::vector<Bytes>& shards) const;
 // Expected CIDs must be from an AUTHENTICATED manifest. A fragment with
 // mismatching CID is discarded as missing; insufficient valid shards fail.
 bool recover(const std::vector<std::optional<Bytes>>& available,
              const std::vector<std::string>& authenticated_cids,
              std::vector<Bytes>& original_data) const;
private:
 std::size_t k_,m_;
 std::uint8_t coefficient(std::size_t row,std::size_t col) const noexcept;
};
} // namespace meshcloud::erasure
