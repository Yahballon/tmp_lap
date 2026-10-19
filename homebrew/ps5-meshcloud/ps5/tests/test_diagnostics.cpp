// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "diagnostics.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main() {
 const auto root=fs::temp_directory_path()/
  ("meshcloud-ps5-probe-"+std::to_string(
   std::chrono::steady_clock::now().time_since_epoch().count()));
 fs::create_directories(root/"files");
 {
  std::ofstream f(root/"files"/"alpha.bin",std::ios::binary);
  f<<"12345";
 }
 {
  std::ofstream f(root/"files"/"beta.dat",std::ios::binary);
  f<<"0123456789";
 }
 fs::create_directory(root/"files"/"folder");
 std::error_code ec;
 fs::create_symlink(root/"files"/"alpha.bin",root/"files"/"link.bin",ec);
 auto entries=meshps5::list_regular_files((root/"files").string());
 assert(entries.size()==2);
 assert(entries[0].name=="alpha.bin"&&entries[0].bytes==5);
 assert(entries[1].name=="beta.dat"&&entries[1].bytes==10);
 assert(meshps5::list_regular_files((root/"missing").string()).empty());
 assert(meshps5::list_regular_files((root/"files").string(),1).size()==1);
 assert(meshps5::list_regular_files((root/"files").string(),0).empty());
 auto d=meshps5::disk("Host temporary",root.string());
 assert(d.mounted);
 assert(!d.possible_local_preflight ||
       (d.space_reported&&d.available_bytes>=meshps5::minimum_contribution+
        meshps5::safety_headroom));
 auto missing=meshps5::disk("Unavailable",(root/"no-device").string());
 assert(!missing.mounted&&!missing.space_reported&&!missing.possible_local_preflight);
 assert(meshps5::human_bytes(0)=="0.00 B");
 fs::remove_all(root);
 std::cout<<"PS5 host diagnostic: real mount availability, symlink exclusion, regular file scan, capacity check passed\n";
}
