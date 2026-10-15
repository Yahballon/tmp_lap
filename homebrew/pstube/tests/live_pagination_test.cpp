// SPDX-License-Identifier: GPL-3.0-or-later
#include "pstube.hpp"
#include <chrono>
#include <thread>
#include <iostream>
int main(){
 pstube::Service service;
 if(!service.search("PS5 gaming"))return 1;
 for(int i=0;i<300 && service.snapshot().searching;i++)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
 const auto first=service.snapshot();
 std::cout<<"PSTUBE live search: "<<first.videos.size()<<" videos; continuation="<<first.hasMore<<"\n";
 if(first.videos.empty())return 2;
 if(!first.hasMore)return 3; // A provider may not supply continuation in some regions.
 if(!service.load_more())return 4;
 for(int i=0;i<300 && service.snapshot().loadingMore;i++)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
 const auto next=service.snapshot();
 std::cout<<"PSTUBE page 2: "<<next.videos.size()<<" total, status="<<next.status<<"\n";
 return next.videos.size()>first.videos.size()?0:5;
}
