// SPDX-License-Identifier: GPL-3.0-or-later
// End-to-end native C++ PS5 stream selection + actual FFmpeg HTTPS decoding.
// Only public test media, with signed source URLs never printed.
#include "pstube.hpp"
#include "media.hpp"
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>
int main(){
 pstube::Service service;
 pstube::MediaPlayer player;
 for(int quality:{720,1080}){
  service.set_quality(quality);
  pstube::Video video;
  video.id="dQw4w9WgXcQ";
  video.title="Public test video";
  if(!service.start_stream(video))return 2;
  pstube::Snapshot snapshot;
  bool finished=false;
  for(int i=0;i<100;i++){
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
   snapshot=service.snapshot();
   if(!snapshot.streamResolving){finished=true;break;}
  }
  if(!finished||snapshot.streamHeight!=quality||!snapshot.streamAdaptive||
    snapshot.streamUrl.empty()||snapshot.streamAudioUrl.empty()){
    std::fprintf(stderr,"Remote HD resolver %dp FAILED: %.180s\n",
       quality,snapshot.streamError.c_str());return 3;
  }
  if(!player.openAdaptiveStream(snapshot.streamUrl,snapshot.streamAudioUrl)){
    std::fprintf(stderr,"Remote HD %dp: opening dual stream rejected\n",quality);
    return 4;
  }
  std::vector<unsigned char> frame;
  int width=0,height=0;uint64_t seq=0;
  bool decoded=false;
  for(int i=0;i<230;i++){
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
   auto state=player.snapshot();
   if(player.pull(frame,width,height,seq)&&height==quality&&width>600){
    decoded=true;break;
   }
   if(state.ended&&!state.playing){
    std::fprintf(stderr,"Remote HD %dp: ffmpeg error: %.180s\n",
        quality,state.message.c_str());break;
   }
  }
  auto state=player.snapshot();
  player.close();service.cancel_stream();
  if(!decoded){
   std::fprintf(stderr,"Remote HD %dp: no decoded frame, state %.180s, output %dx%d\n",
     quality,state.message.c_str(),width,height);return 5;
  }
  std::printf("Remote HD playback: native FFmpeg decoded %dx%d video with independent AAC input\n",
      width,height);
 }
 return 0;
}
