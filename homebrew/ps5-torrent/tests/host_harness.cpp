// Host integration harness: drives exactly the same C++ engine as the PS5 binary.
#include "torrent_engine.hpp"
#include <chrono>
#include <string>
#include <iostream>
#include <thread>
int main(int argc,char** argv){
 if(argc<2||argc>3)return 2;
 bool cancelTest=argc==3&&std::string(argv[2])=="--cancel";
 auto& e=torrent::engine();
 if(!e.start(argv[1]))return 3;
 if(cancelTest){
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  const auto started=std::chrono::steady_clock::now();
  e.stop();
  for(int i=0;i<60;i++){
   auto state=e.snapshot();
   if(!state.running){
    const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
    std::cout<<"CANCEL elapsed="<<elapsed<<" status="<<state.status<<std::endl;
    return !state.done&&elapsed<2.5 ? 0:6;
   }
   std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  std::cerr<<"Cancel did not finish promptly"<<std::endl;return 7;
 }
 for(int i=0;i<600;i++){
  auto state=e.snapshot();
  if(!state.running){
   std::cout<<"RESULT done="<<state.done<<" verified="<<state.verifiedPieces
            <<" total="<<state.pieceCount<<" bytes="<<state.completedBytes
            <<" status="<<state.status<<std::endl;
   return state.done&&state.completedBytes==state.totalBytes ? 0:4;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
 }
 e.stop();
 std::cerr<<"Host test download exceeded 60s"<<std::endl;
 return 5;
}
