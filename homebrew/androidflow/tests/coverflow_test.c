#include "../native/yah_coverflow_math.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
  assert(yah_wrap(-1,5)==4 && yah_wrap(5,5)==0 && yah_wrap(0,0)==0);
  assert(yah_wrap_offset(9.f,5)==-1.f && yah_wrap_offset(-9.f,5)==1.f);
  yah_pose c=yah_cover_pose(0.f,0.f),l=yah_cover_pose(-1.f,0.f),r=yah_cover_pose(1.f,0.f);
  assert(fabsf(c.x-960.f)<.01f);
  assert(fabsf(l.x+r.x-1920.f)<.01f);
  assert(c.scale>l.scale && c.scale>r.scale);
  assert(l.yaw<0 && r.yaw>0 && l.brightness<c.brightness);
  for(int n=1;n<=64;n++)for(int k=-130;k<=130;k++){
    int w=yah_wrap(k,n);
    assert(w>=0 && w<n);
  }
  puts("PASS AndroidFlow Coverflow geometry/navigation");
  return 0;
}
