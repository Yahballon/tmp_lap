// Host integration harness: drives exactly the same C++ engine as the PS5 binary.
#include "torrent_engine.hpp"
#include <chrono>
#include <iostream>
#include <thread>
int main(int argc,char** argv){
 if(argc!=2)return 2;
 auto& e=torrent::engine();
 if(!e.start(argv[1]))return 3;
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
