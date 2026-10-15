// SPDX-License-Identifier: GPL-3.0-or-later
#include "pstube.hpp"
#include "range_media.hpp"
#include <chrono>
#include <thread>
#include <iostream>
int main(){
 pstube::Service service;service.set_quality(720);
 pstube::Video video;video.id="dQw4w9WgXcQ";
 if(!service.start_stream(video))return 1;
 pstube::Snapshot state;
 for(int i=0;i<130;i++){
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  state=service.snapshot();if(!state.streamResolving)break;
 }
 if(state.streamUrl.empty()||state.streamAudioUrl.empty())return 2;
 for(const auto& url:{state.streamUrl,state.streamAudioUrl}){
  for(auto end:{4095ULL,262143ULL}){
   std::string error;
   const bool ok=pstube::probe_bounded_media_range(url,end,error);
   std::cout<<"NATIVE HD RANGE "<<(end+1)<<" bytes "<<(ok?"OK":"FAILED")<<" "<<error<<std::endl;
  }
 }
 return 0;
}
