// SPDX-License-Identifier: GPL-3.0-or-later
// Native PS5-compatible HTTPS implementation: actual search through five pages.
#include "pstube.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <unistd.h>
int main(){
 pstube::Service svc;
 if(!svc.search("PS5"))return 1;
 pstube::Snapshot snap;
 for(int i=0;i<130;i++){
   usleep(200000);snap=svc.snapshot();
   if(!snap.searching)break;
 }
 std::cout<<"SEARCH PAGE 1: "<<snap.videos.size()<<" has_more="<<snap.hasMore
    <<" status="<<snap.status<<"\n"<<std::flush;
 if(snap.searching||snap.videos.size()<10||!snap.hasMore)return 2;
 for(int page=2;page<=5;++page){
   const size_t prior=snap.videos.size();
   if(!svc.load_more()){
    std::cerr<<"FAILED: page "<<page<<" could not launch; "<<snap.status<<"\n";return 3;
   }
   for(int i=0;i<160;i++){
     usleep(200000);snap=svc.snapshot();
     if(!snap.loadingMore)break;
   }
   std::cout<<"SEARCH PAGE "<<page<<": "<<snap.videos.size()
       <<" has_more="<<snap.hasMore<<" status="<<snap.status<<"\n"<<std::flush;
   if(snap.loadingMore||snap.videos.size()<=prior)return 4;
   if(page<5&&!snap.hasMore)return 5;
 }
 std::set<std::string> ids;
 for(auto& v:snap.videos)if(!ids.insert(v.id).second)return 6;
 if(ids.size()<65)return 7;
 std::cout<<"SEARCH PAGINATION: PASS "<<ids.size()<<" unique videos across 5 pages\n";
 return 0;
}
