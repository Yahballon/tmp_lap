// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
namespace audio_cover {
// Returns only complete encoded JPEG/PNG/GIF bytes from the M4B attached picture.
// Never decode or upload a GL texture on worker threads.
bool extract(const std::string& file,std::string& encoded,std::string& diagnostic);
}
