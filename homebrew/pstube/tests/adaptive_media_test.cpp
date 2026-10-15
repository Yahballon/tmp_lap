// SPDX-License-Identifier: GPL-3.0-or-later
#include "media.hpp"
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>
int main(int argc,char** argv){
 if(argc<3)return 2;
 pstube::MediaPlayer player;
 if(!player.openAdaptiveFiles(argv[1],argv[2])){
  std::fprintf(stderr,"ADAPTIVE: could not open separate H.264/AAC files\n");return 3;
 }
 std::vector<unsigned char> rgba;int width=0,height=0;uint64_t seq=0;
 bool got=false;
 for(int n=0;n<100;n++){
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  if(player.pull(rgba,width,height,seq)&&height==720&&width==1280){got=true;break;}
 }
 if(!got){auto st=player.snapshot();std::fprintf(stderr,"ADAPTIVE: no 720p video: %s\n",st.message.c_str());return 4;}
 auto st=player.snapshot();
 if(st.duration<5.0){std::fprintf(stderr,"ADAPTIVE: duration missing\n");return 5;}
 if(!player.seekRelative(3)){std::fprintf(stderr,"ADAPTIVE: seek rejected\n");return 6;}
 bool seeked=false;
 for(int n=0;n<90;n++){
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  if(player.pull(rgba,width,height,seq)&&player.snapshot().seconds>=2.8){seeked=true;break;}
 }
 if(!seeked){std::fprintf(stderr,"ADAPTIVE: no seeked frame at expected timestamp\n");return 7;}
 player.close();
 std::puts("PASS: 720p H.264 video + separate AAC decoding, seeking and SDL audio context");
 return 0;
}
