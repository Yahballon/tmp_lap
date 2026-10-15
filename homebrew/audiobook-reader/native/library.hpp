// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace audio {
struct Book {std::string path,title,author;int64_t bytes=0;double position=0,duration=0;bool favorite=false;};
std::string clean_title(std::string path);
std::string scan_report();
void scan(const std::vector<std::string>& roots,std::vector<Book>& out);
void load(std::vector<Book>& out);
bool save(const std::vector<Book>& out);
std::string root();
}
