// SPDX-License-Identifier: GPL-3.0-or-later
// Host integration diagnostic of the exact native C++ resolver + transfer code.
#include "pstube.hpp"
#include "transfer.hpp"
#include <atomic>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/stat.h>
int main(){
 std::string url,error;
 if(!pstube::resolve_android_mp4("dQw4w9WgXcQ",url,error)){
  std::cerr<<"NATIVE DOWNLOAD: cannot resolve: "<<error<<"\n";return 1;
 }
 const std::string target="/tmp/pstube-native-stream-test.mp4";
 std::atomic<int> percent{0};
 const bool downloaded=pstube::fetch_mp4(url,target,percent,error);
 if(!downloaded){
  std::cerr<<"NATIVE DOWNLOAD: failed: "<<error<<"\n";return 2;
 }
 FILE* file=std::fopen(target.c_str(),"rb");
 unsigned char header[32]{};
 bool mp4=file&&std::fread(header,1,sizeof(header),file)==sizeof(header);
 if(file)std::fclose(file);
 bool found=false;
 for(int i=0;i<27;i++)if(header[i]=='f'&&header[i+1]=='t'&&header[i+2]=='y'&&header[i+3]=='p')found=true;
 struct stat st{};if(stat(target.c_str(),&st)!=0)st.st_size=0;
 remove(target.c_str());
 if(!mp4||!found||st.st_size<4096){
  std::cerr<<"NATIVE DOWNLOAD: invalid MP4 data\n";return 3;
 }
 std::cout<<"NATIVE DOWNLOAD: verified "<<st.st_size<<" bytes, progress "<<percent.load()<<"%\n";
 return 0;
}
