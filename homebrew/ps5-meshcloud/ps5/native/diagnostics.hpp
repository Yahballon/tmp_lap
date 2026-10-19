// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace meshps5 {
constexpr std::uint64_t minimum_contribution=13ULL*1024*1024*1024;
constexpr std::uint64_t safety_headroom=2ULL*1024*1024*1024;
struct Disk {
 std::string name,path;
 bool mounted=false,space_reported=false;
 std::uint64_t available_bytes=0;
 bool possible_local_preflight=false;
};
struct File {
 std::string name;
 std::uint64_t bytes=0;
};
struct Snapshot {
 std::vector<Disk> disks;
 std::vector<File> files;
 std::string file_folder;
 std::string error;
};
// Read-only, platform-neutral filesystem probing; no peer networking,
// identity auditing, disk allocation or privilege-specific kernel APIs.
Disk disk(const std::string& name,const std::string& path);
std::vector<File> list_regular_files(const std::string& folder,std::size_t max_count=350);
Snapshot collect(int selected_disk=0);
std::string human_bytes(std::uint64_t bytes);
} // namespace meshps5
