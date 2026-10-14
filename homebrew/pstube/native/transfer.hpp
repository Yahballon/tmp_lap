// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <cstdint>
#include <string>
namespace pstube {
// TLS-verified download into an atomic .part file; never treats incomplete HTTP as success.
bool fetch_mp4(const std::string& url,const std::string& target,
               std::atomic<int>& percent,std::string& error);
}