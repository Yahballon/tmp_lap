// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
namespace audio_cover {
// Read MP4/iTunes moov.udta.meta.ilst.covr.data payload without decoding audio.
// Useful when FFmpeg does not expose covr as an attached-picture stream.
bool extract_mp4_covr(const std::string& filename,std::string& cover);
}
