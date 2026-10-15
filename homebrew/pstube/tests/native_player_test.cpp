// SPDX-License-Identifier: GPL-3.0-or-later
#include "pstube.hpp"
#include <cassert>
#include <iostream>
#include <string>
int main(){
 std::string media,err;
 // This is publicly accessible test content; URLs are ephemeral and must never be logged.
 const bool ok=pstube::resolve_android_mp4("dQw4w9WgXcQ",media,err);
 if(!ok){
  std::cerr<<"NATIVE RESOLVER: "<<err<<"\n";
  return 1;
 }
 if(media.rfind("https://",0)!=0 || media.size()<60){
  std::cerr<<"NATIVE RESOLVER: Invalid media URL\n";return 2;
 }
 std::cout<<"NATIVE RESOLVER: Android API provided signed HTTPS MP4 URL (redacted)\n";
 std::string invalid,reason;
 assert(!pstube::resolve_android_mp4("../../passwd",invalid,reason));
 return 0;
}
