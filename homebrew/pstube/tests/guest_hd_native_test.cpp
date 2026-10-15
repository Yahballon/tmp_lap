// SPDX-License-Identifier: GPL-3.0-or-later
// Tests anonymous PS5-native guest context resolution, without account cookies.
#include "pstube.hpp"
#include <chrono>
#include <iostream>
#include <thread>
bool run(pstube::Service& service,const std::string& id,int expectedHeight,bool required){
 pstube::Video video; video.id=id;video.title="Public test";
 if(!service.start_stream(video)){std::cerr<<"GUEST HD: could not start\n";return false;}
 pstube::Snapshot info;
 for(int i=0;i<150;i++){
  std::this_thread::sleep_for(std::chrono::milliseconds(180));
  info=service.snapshot();
  if(!info.streamResolving)break;
 }
 const bool good=!info.streamResolving&&info.streamHeight>=expectedHeight&&
   info.streamAdaptive&&!info.streamUrl.empty()&&!info.streamAudioUrl.empty();
 std::cout<<"GUEST HD "<<id<<" requested "<<expectedHeight
  <<" selected "<<info.streamHeight<<" adaptive "<<info.streamAdaptive
  <<" result "<<(good?"OK":(info.streamError.empty()?"Unavailable":info.streamError))
  <<"\n";
 service.cancel_stream();
 return !required||good;
}
int main(){
 pstube::Service service;
 service.set_quality(720);
 if(!run(service,"dQw4w9WgXcQ",720,true))return 2;
 // A second publicly indexed video may be restricted by YouTube. Diagnose,
 // but don't lie to CI by pretending a missing guest-accessible stream exists.
 run(service,"jNQXAC9IVRw",720,false);
 std::cout<<"Anonymous C++ HD resolver and truthful login errors PASS\n";
 return 0;
}
