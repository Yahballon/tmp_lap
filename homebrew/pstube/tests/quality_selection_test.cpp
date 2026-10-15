// SPDX-License-Identifier: GPL-3.0-or-later
#include "pstube.hpp"
#include <cassert>
#include <iostream>
int main(){
 const std::string json=R"JSON({"playabilityStatus":{"status":"OK"},"streamingData":{
 "formats":[{"itag":18,"mimeType":"video/mp4; codecs=\"avc1.42001E, mp4a.40.2\"",
 "url":"https://r3.googlevideo.com/videoplayback?id=ONE","qualityLabel":"360p","height":360,"bitrate":650000}],
 "adaptiveFormats":[
 {"itag":136,"mimeType":"video/mp4; codecs=\"avc1.4d401f\"",
 "url":"https://r3.googlevideo.com/videoplayback?id=TWO","height":720,"bitrate":2600000},
 {"itag":137,"mimeType":"video/mp4; codecs=\"avc1.640028\"",
 "url":"https://r3.googlevideo.com/videoplayback?id=THREE","height":1080,"bitrate":5000000},
 {"itag":140,"mimeType":"audio/mp4; codecs=\"mp4a.40.2\"",
 "url":"https://r3.googlevideo.com/videoplayback?id=AUDIO","bitrate":128000},
 {"itag":251,"mimeType":"audio/webm; codecs=\"opus\"",
 "url":"https://r3.googlevideo.com/videoplayback?id=BAD","bitrate":160000}
 ]}})JSON";
 pstube::StreamSelection s;std::string e;
 assert(pstube::select_stream_formats(json,0,s,e)&&s.height==720&&s.adaptive&&!s.audioUrl.empty());
 assert(pstube::select_stream_formats(json,360,s,e)&&s.height==360&&!s.adaptive&&s.audioUrl.empty());
 assert(pstube::select_stream_formats(json,720,s,e)&&s.height==720&&s.adaptive);
 assert(pstube::select_stream_formats(json,1080,s,e)&&s.height==1080&&s.adaptive);
 const std::string noAudio=R"JSON({"streamingData":{
 "formats":[{"itag":18,"mimeType":"video/mp4","url":"https://r3.googlevideo.com/a","height":360}],
 "adaptiveFormats":[{"itag":137,"mimeType":"video/mp4; codecs=\"avc1.640028\"",
 "url":"https://r3.googlevideo.com/b","height":1080}]}})JSON";
 assert(pstube::select_stream_formats(noAudio,1080,s,e)&&s.height==360&&!s.adaptive);
 assert(!pstube::select_stream_formats("{}",1080,s,e));
 std::cout<<"PSTUBE quality selection: Auto 720p, SD, 720p, 1080p and safe fallback PASS\n";
}
