// SPDX-License-Identifier: GPL-3.0-or-later
// Exercises the same native C++ HTTPS search and continuation paths as the PS5.
#include "pstube.hpp"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <set>
#include <unistd.h>
int main(){
 pstube::Service svc;
 if(!svc.search("PS5")){std::cerr<<"PAGINATION: search not launched\n";return 1;}
 pstube::Snapshot snap;
 for(int i=0;i<150;++i){
  usleep(200000);snap=svc.snapshot();
  if(!snap.searching)break;
 }
 std::cout<<"PAGINATION INITIAL: "<<snap.videos.size()<<" videos; hasMore="<<snap.hasMore
  <<" pages="<<snap.pagesLoaded<<" status="<<snap.status<<"\n";
 if(snap.searching || snap.videos.size()<10 || !snap.hasMore)return 2;
 const auto firstSize=snap.videos.size();
 if(!svc.load_more()){std::cerr<<"PAGINATION: did not start continuation\n";return 3;}
 for(int i=0;i<150;++i){
  usleep(200000);snap=svc.snapshot();
  if(!snap.searching)break;
 }
 std::cout<<"PAGINATION NEXT: "<<snap.videos.size()<<" videos; hasMore="<<snap.hasMore
  <<" pages="<<snap.pagesLoaded<<" status="<<snap.status<<"\n";
 if(snap.searching || snap.videos.size()<=firstSize || snap.pagesLoaded<2)return 4;
 std::set<std::string> unique;
 for(const auto& v:snap.videos)if(!unique.insert(v.id).second)return 5;
 return 0;
}
