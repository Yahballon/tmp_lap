// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <string>
namespace pstube {
// Download a verified HTTPS MP4/M4A from a trusted googlevideo CDN in finite
// HTTP byte ranges. Atomic rename prevents incomplete cache entries.
// Unlike the ordinary progressive downloader, each request requires HTTP 206.
bool fetch_bounded_range_media(const std::string& url,const std::string& target,
 std::atomic<int>& percent,std::atomic<bool>& cancel,std::string& error);
}
