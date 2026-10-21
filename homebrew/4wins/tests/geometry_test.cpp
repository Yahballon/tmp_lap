#include "../native/board_layout.hpp"
#include <cassert>
#include <cstdio>
using namespace wins4;
int main(){
 assert(hole_x(0)==648.f && hole_x(6)==1297.f);
 assert(hole_y(0)==759.f && hole_y(5)==288.f);
 for(int x=0;x<7;x++)for(int y=0;y<6;y++){
   if(x<6)assert(hole_x(x+1)>hole_x(x)+100.f);
   if(y<5)assert(hole_y(y+1)<hole_y(y)-85.f);
 }
 assert(kDiscSize < 80.f && kDiscSize > 70.f);
 puts("PASS: 42 photo-calibrated hole positions, gravity orientation and disc clearance");
}
