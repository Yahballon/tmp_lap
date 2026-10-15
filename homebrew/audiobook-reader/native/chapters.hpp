// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
namespace audio {
struct Chapter {std::string title;double start=0,end=0;};
std::vector<Chapter> chapters(const std::string& path,double* duration=nullptr,std::string* author=nullptr,std::string* title=nullptr);
}
