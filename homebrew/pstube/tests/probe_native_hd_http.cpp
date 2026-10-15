// SPDX-License-Identifier: GPL-3.0-or-later
// Writes ephemeral signed URLs only to mode-0600 local files for CI diagnostics.
// Never outputs secrets, media URLs, tokens or query strings to workflow logs.
#include "pstube.hpp"
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <thread>
int main(){
 pstube::Service svc;svc.set_quality(720);
 pstube::Video video;video.id="dQw4w9WgXcQ";video.title="Test clip";
 if(!svc.start_stream(video))return 1;
 pstube::Snapshot snap;
 for(int i=0;i<130;i++){
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  snap=svc.snapshot();if(!snap.streamResolving)break;
 }
 if(snap.streamHeight!=720||!snap.streamAdaptive||
   snap.streamUrl.empty()||snap.streamAudioUrl.empty())return 2;
 umask(0077);
 FILE* out=std::fopen("/tmp/pstube-hd-stream-urls","wb");
 if(!out)return 3;
 bool ok=std::fprintf(out,"%s\n%s\n",snap.streamUrl.c_str(),
    snap.streamAudioUrl.c_str())>0;
 if(std::fclose(out)!=0)ok=false;
 return ok?0:4;
}
