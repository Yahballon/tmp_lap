// SPDX-License-Identifier: GPL-3.0-or-later
#include "pstube.hpp"
#include <cassert>
#include <iostream>
#include <string>
int main(){
 // English original and a louder German AI dub: bitrate/default must NOT
 // automatically override original speech.
 const std::string multi=R"JSON({"streamingData":{
  "formats":[{"itag":18,"mimeType":"video/mp4; codecs=\"avc1.42001E, mp4a.40.2\"",
   "url":"https://r3.googlevideo.com/progressive","height":360,"bitrate":650000}],
  "adaptiveFormats":[
   {"itag":136,"mimeType":"video/mp4; codecs=\"avc1.4d401f\"",
    "url":"https://r3.googlevideo.com/720","height":720,"bitrate":3500000},
   {"itag":137,"mimeType":"video/mp4; codecs=\"avc1.640028\"",
    "url":"https://r3.googlevideo.com/1080","height":1080,"bitrate":5500000},
   {"itag":140,"mimeType":"audio/mp4; codecs=\"mp4a.40.2\"",
    "url":"https://r3.googlevideo.com/dub-de","bitrate":192000,
    "audioTrack":{"id":"de.1","displayName":"German dubbed","audioIsDefault":true}},
   {"itag":140,"mimeType":"audio/mp4; codecs=\"mp4a.40.2\"",
    "url":"https://r3.googlevideo.com/original-en","bitrate":128000,
    "audioTrack":{"id":"en.0","displayName":"English original","audioIsDefault":false}},
   {"itag":140,"mimeType":"audio/mp4; codecs=\"mp4a.40.2\"",
    "url":"https://r3.googlevideo.com/french-fr","bitrate":96000,
    "audioTrack":{"id":"fr.1","displayName":"French"}}
 ]}})JSON";
 pstube::StreamSelection x;std::string error;
 assert(pstube::select_stream_formats(multi,720,x,error,"original"));
 assert(x.adaptive && x.height==720 && x.audioUrl.find("original-en")!=std::string::npos);
 assert(x.audioLanguage=="en"&&x.audioOriginal);
 assert(pstube::select_stream_formats(multi,1080,x,error,"de"));
 assert(x.height==1080&&x.audioUrl.find("dub-de")!=std::string::npos);
 assert(x.audioLanguage=="de");
 assert(pstube::select_stream_formats(multi,720,x,error,"fr"));
 assert(x.audioLanguage=="fr"&&x.audioUrl.find("french-fr")!=std::string::npos);
 assert(pstube::select_stream_formats(multi,720,x,error,"ja"));
 assert(x.audioLanguage=="en"&&x.audioOriginal); // unavailable => original
 assert(!pstube::select_stream_formats(multi,720,x,error,"untrusted_value"));
 // Backward compatibility: ordinary single-AAC video without track metadata.
 const std::string simple=R"JSON({"streamingData":{"adaptiveFormats":[
 {"mimeType":"video/mp4; codecs=\"avc1.4d401f\"","height":720,
  "url":"https://r3.googlevideo.com/video","bitrate":2000000},
 {"mimeType":"audio/mp4; codecs=\"mp4a.40.2\"",
  "url":"https://r3.googlevideo.com/audio","bitrate":128000}
 ]}})JSON";
 assert(pstube::select_stream_formats(simple,720,x,error,"original"));
 assert(x.audioUrl.find("/audio")!=std::string::npos);
 std::cout<<"PASS: Original English over dubbed default, German French, fallback, legacy audio\n";
 return 0;
}
