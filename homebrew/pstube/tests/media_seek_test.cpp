// SPDX-License-Identifier: GPL-3.0-or-later
// Host regression test: the same FFmpeg/SDL worker compiled for the PS5.
// Runs against the bundled, self-generated and appropriately licensed H.264/AAC clip.
#include "media.hpp"
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
int main(int argc,char** argv){
 if(argc!=2)return 2;
 pstube::MediaPlayer player;
 if(!player.open(argv[1])){std::fprintf(stderr,"Local media file unavailable\n");return 3;}
 std::vector<unsigned char> frame;
 int width=0,height=0;
 uint64_t sequence=0;
 bool decoded=false;
 for(int i=0;i<45;i++){
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  if(player.pull(frame,width,height,sequence)){decoded=true;break;}
 }
 if(!decoded||width<300||height<150){std::fprintf(stderr,"No decoded video frames\n");return 4;}
 const auto initial=player.snapshot();
 if(initial.duration<4.5){std::fprintf(stderr,"Duration not detected: %.2f\n",initial.duration);return 5;}
 if(!player.seekRelative(3)){std::fprintf(stderr,"Forward seek rejected\n");return 6;}
 bool afterForward=false;uint64_t priorSequence=sequence;
 for(int i=0;i<45;i++){
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  if(player.pull(frame,width,height,sequence) && sequence>priorSequence &&
       player.snapshot().seconds>=2.6){afterForward=true;break;}
 }
 if(!afterForward){std::fprintf(stderr,"Forward seek produced no new-position frames\n");return 7;}
 if(!player.seekRelative(-2)){std::fprintf(stderr,"Rewind rejected\n");return 8;}
 bool afterRewind=false;priorSequence=sequence;
 for(int i=0;i<45;i++){
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  const auto now=player.snapshot();
  if(player.pull(frame,width,height,sequence) && sequence>priorSequence &&
      now.seconds<2.8){afterRewind=true;break;}
 }
 if(!afterRewind){std::fprintf(stderr,"Rewind produced no earlier-position frames\n");return 9;}
 player.pause();
 if(!player.snapshot().paused){std::fprintf(stderr,"Pause state failed\n");return 10;}
 player.pause();
 player.close();
 std::puts("PASS: Native FFmpeg/SDL video frames, duration, forward seek, rewind and pause");
 return 0;
}
