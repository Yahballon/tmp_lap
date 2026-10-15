// SPDX-License-Identifier: GPL-3.0-or-later
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
 auto first=snap.videos.size();
 if(!svc.load_more())return 3;
 for(int i=0;i<130;i++){
   usleep(200000);snap=svc.snapshot();
   if(!snap.loadingMore)break;
 }
 std::cout<<"SEARCH PAGE 2: "<<snap.videos.size()<<" has_more="<<snap.hasMore
    <<" status="<<snap.status<<"\n"<<std::flush;
 if(snap.loadingMore||snap.videos.size()<=first)return 4;
 for(int page=3;page<=12 && snap.hasMore;++page){
   const auto count=snap.videos.size();
   if(!svc.load_more())return 6;
   for(int i=0;i<130;++i){
     usleep(200000);snap=svc.snapshot();
     if(!snap.loadingMore)break;
   }
   std::cout<<"SEARCH PAGE "<<page<<": "<<snap.videos.size()
     <<" (+"<<(snap.videos.size()-count)<<") has_more="<<snap.hasMore
     <<" status="<<snap.status<<"\\n"<<std::flush;
   if(snap.loadingMore)return 7;
 }
 if(snap.videos.size()<=first+5)return 8;
 std::set<std::string> ids;
 for(auto& v:snap.videos)if(!ids.insert(v.id).second)return 5;
 std::cout<<"SEARCH PAGINATION: PASS "<<snap.videos.size()<<" distinct real videos\n";
 return 0;
}
