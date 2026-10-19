// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <filesystem>
#include <istream>
#include <string>
namespace meshcloud::cid {
// Incremental SHA2-256 CIDv1 for streams/files: constant memory (~1 MiB).
// Returns false on I/O failure / byte-counter overflow; no arbitrary size limit.
bool from_stream(std::istream& stream,std::string& result,std::uint64_t& total_bytes);
bool from_file(const std::filesystem::path& path,std::string& result,std::uint64_t& total_bytes);
}
