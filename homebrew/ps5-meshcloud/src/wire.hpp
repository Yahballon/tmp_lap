// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "core.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
namespace meshcloud::wire {
// Transport-neutral framing for already authenticated TLS/Noise sessions.
// Fixed v1 header: 4 magic bytes, version (1), operation (1), reserved (2),
// unsigned big-endian payload length (4), then payload bytes.
// NEVER deserialize unbounded frames or dispatch before membership verification.
constexpr std::size_t header_bytes=12;
constexpr std::size_t max_payload_bytes=4*1024*1024+4096;
struct Frame {
 Operation operation=Operation::Metadata;
 std::vector<std::uint8_t> payload;
};
bool serialize(const Frame&,std::vector<std::uint8_t>& wire);
bool authenticated_parse(const std::vector<std::uint8_t>& bytes,
                         const VerifiedMembership& verified_session,
                         std::uint64_t now,Frame& decoded);
} // namespace meshcloud::wire
