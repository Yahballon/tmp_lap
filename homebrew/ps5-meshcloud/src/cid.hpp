// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace meshcloud::cid {
// CIDv1: multibase base32lower ('b') of unsigned-varint CID version=1,
// multicodec raw=0x55, multihash sha2-256=0x12, digest length=32.
// This is a content ID for raw bytes, NOT a network locator or capability key.
std::string from_bytes(const std::vector<std::uint8_t>& bytes);
bool validate(const std::string& cid);
bool matches(const std::string& cid,const std::vector<std::uint8_t>& bytes);
} // namespace meshcloud::cid
