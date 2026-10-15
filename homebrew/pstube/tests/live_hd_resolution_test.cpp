// SPDX-License-Identifier: GPL-3.0-or-later
// End-to-end quality check through the same native PS5 C++ Service implementation.
#include "pstube.hpp"
#include <chrono>
#include <iostream>
#include <thread>
int main(){
 pstube::Service svc;
 for(int q:{720,1080}){
  svc.set_quality(q);
  pstube::Video video;video.id="dQw4w9WgXcQ";video.title="Test public video";
  if(!svc.start_stream(video)){std::cerr<<"HD STREAM: resolver failed to start\n";return 1;}
  pstube::Snapshot snap;
  for(int i=0;i<130;i++){
   std::this_thread::sleep_for(std::chrono::milliseconds(180));
   snap=svc.snapshot();
   if(!snap.streamResolving)break;
  }
  std::cout<<"HD STREAM: selected "<<q<<"p -> "<<snap.streamHeight
      <<"p adaptive="<<snap.streamAdaptive<<" videoUrl="<<!snap.streamUrl.empty()
      <<" audioUrl="<<!snap.streamAudioUrl.empty()
      <<" err="<<snap.streamError.substr(0,100)<<"\n";
  if(snap.streamResolving||snap.streamHeight!=q||!snap.streamAdaptive||
     snap.streamUrl.find("googlevideo.com")==std::string::npos||
     snap.streamAudioUrl.find("googlevideo.com")==std::string::npos)return 2;
  svc.cancel_stream();
 }
 std::cout<<"NATIVE STREAM RESOLVER: 720p + 1080p URLs with AAC PASS\n";
 return 0;
}
