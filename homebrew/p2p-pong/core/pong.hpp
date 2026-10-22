#pragma once
#include <algorithm>
#include <cmath>
namespace pong {
struct State { float left=0.5f,right=0.5f,x=0.5f,y=0.5f,vx=0.55f,vy=0.28f; int score_l=0,score_r=0; };
inline void reset(State& s,int direction){s.x=s.y=.5f;s.vx=.55f*(direction>=0?1.f:-1.f);s.vy=.28f;}
inline void step(State& s,float dt,float left_input,float right_input){
 dt=std::clamp(dt,0.f,.05f);
 s.left=std::clamp(s.left+left_input*dt*.95f,.105f,.895f);
 s.right=std::clamp(s.right+right_input*dt*.95f,.105f,.895f);
 s.x+=s.vx*dt;s.y+=s.vy*dt;
 if(s.y<.025f){s.y=.025f;s.vy=std::fabs(s.vy);}
 if(s.y>.975f){s.y=.975f;s.vy=-std::fabs(s.vy);}
 if(s.vx<0 && s.x<=.065f && s.x>=.03f && std::fabs(s.y-s.left)<.115f){s.x=.065f;s.vx=std::fabs(s.vx)*1.035f;s.vy+=(s.y-s.left)*1.2f;}
 if(s.vx>0 && s.x>=.935f && s.x<=.97f && std::fabs(s.y-s.right)<.115f){s.x=.935f;s.vx=-std::fabs(s.vx)*1.035f;s.vy+=(s.y-s.right)*1.2f;}
 if(s.x<0){++s.score_r;reset(s,1);}
 if(s.x>1){++s.score_l;reset(s,-1);}
}
}
