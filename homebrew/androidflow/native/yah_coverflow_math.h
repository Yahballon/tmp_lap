// GBAXY-inspired shallow-angle Coverflow geometry for square Android app covers.
// Adapted from PS5GBAX ps5/frontend/fe_coverflow_layout.h. GPL-3.0-or-later.
#ifndef YAH_COVERFLOW_MATH_H
#define YAH_COVERFLOW_MATH_H
#include <math.h>
typedef struct { float x,scale,yaw,depth,brightness; } yah_pose;
static inline float yah_clamp(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
static inline float yah_smooth(float v){v=yah_clamp(v,0.f,1.f);return v*v*(3.f-2.f*v);}
static inline yah_pose yah_cover_pose(float offset,float t){
  float a=fabsf(offset),sign=offset<0.f?-1.f:1.f;
  float side=yah_smooth(a),tail=fmaxf(a-1.f,0.f);
  yah_pose p;
  p.x=960.f+sign*(342.f*fminf(a,1.f)+284.f*tail);
  p.scale=a<=1.f?1.25f-0.19f*side:fmaxf(0.58f,1.06f-0.12f*tail);
  p.yaw=sign*0.27f*side+(1.f-side)*0.018f*sinf(t*0.7f);
  p.depth=side*(105.f+80.f*tail);
  p.brightness=fmaxf(0.48f,1.f-0.13f*side-0.07f*tail);
  return p;
}
static inline int yah_wrap(int n,int count){return count>0?(n%count+count)%count:0;}
static inline float yah_wrap_offset(float offset,int count){
  if(count<=1)return 0.f;
  while(offset>count*0.5f)offset-=(float)count;
  while(offset< -count*0.5f)offset+=(float)count;
  return offset;
}
#endif
