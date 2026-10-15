// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Pure playback-time chapter lookup. The highlighted chapter must track
// the audio clock, not only the last chapter manually selected with the pad.
#include "chapters.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
namespace audio {
inline int chapter_at(const std::vector<Chapter>& chapters,double seconds){
 if(chapters.empty())return -1;
 if(!std::isfinite(seconds))return 0;
 const double position=std::max(0.0,seconds);
 // MP4 chapters are sorted by their start time in chapters.cpp.
 const auto after=std::upper_bound(chapters.begin(),chapters.end(),position,
  [](double value,const Chapter& chapter){return value<chapter.start;});
 if(after==chapters.begin())return 0;
 return static_cast<int>((after-chapters.begin())-1);
}
} // namespace audio
