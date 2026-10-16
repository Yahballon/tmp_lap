// SPDX-License-Identifier: GPL-3.0-or-later
// Host-side contract test: the same native OpenSSL transport used on PS5.
// Downloads a small public HD MP4 to /tmp; never redistributes the clip.
#include "motion_download.hpp"
#include <atomic>
#include <cstdio>
#include <string>
int main(){
 const char* url="https://motionbgs.com/dl/hd/737/";
 const std::string path="/tmp/themechanger-motionbgs-probe.mp4";
 std::atomic<int> percent{0};std::string error;
 const bool ok=motion::fetch_file(url,path,percent,error);
 if(!ok){std::fprintf(stderr,"HTTPS HD download failed: %s\n",error.c_str());return 1;}
 FILE* f=std::fopen(path.c_str(),"rb");if(!f)return 2;
 unsigned char header[8]{};bool good=std::fread(header,1,8,f)==8;
 std::fclose(f);std::remove(path.c_str());
 good=good&&header[4]=='f'&&header[5]=='t'&&header[6]=='y'&&header[7]=='p';
 std::printf("MotionBGS native HTTPS: %s, progress=%d\n",good?"MP4 OK":"invalid",percent.load());
 return good&&percent==100?0:3;
}
