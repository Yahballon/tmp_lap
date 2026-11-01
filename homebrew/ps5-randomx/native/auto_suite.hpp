// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
namespace rxbench {
struct AutoResult {
 int profile=-1,outcome=0;
 uint64_t hashes=0,elapsed_us=0;
 double hps=0.0;
};
struct AutoSuite {
 enum Kind:int { All10=0, Light4=1, LightFull5=2, SecureJit3=3 };
 bool active=false,done=false,cancelled=false,with_full=false;
 Kind kind=All10;
 int step=0,recorded=0,total=0,duration=30;
 int plan[10]{};
 AutoResult results[10]{};
 void begin(bool full,int seconds){begin_kind(full?LightFull5:Light4,seconds);}
 void begin_jit(int seconds){begin_kind(SecureJit3,seconds);}
 void begin_all(int seconds){begin_kind(All10,seconds);}
 void begin_kind(Kind k,int seconds){
   active=true;done=false;cancelled=false;kind=k;
   with_full=(k==All10||k==LightFull5);
   step=0;recorded=0;duration=seconds;
   const int all[10]={0,1,2,3,7,10,11,12,13,14};
   const int light[5]={0,1,2,3,4};
   const int jit[3]={7,10,11};
   total=k==All10?10:k==SecureJit3?3:k==LightFull5?5:4;
   for(int i=0;i<10;i++){
     plan[i]=k==All10?all[i]:k==SecureJit3&&i<3?jit[i]:i<5?light[i]:-1;
     results[i]=AutoResult{};
   }
 }
 int current_profile()const{return step<total?plan[step]:-1;}
 void cancel(){if(active)cancelled=true;}
 bool record(int outcome,bool completed,uint64_t hashes,uint64_t us){
   if(!active||recorded!=step||step>=total)return false;
   AutoResult &r=results[step];
   r.profile=plan[step];r.outcome=outcome;r.hashes=hashes;r.elapsed_us=us;
   r.hps=us?double(hashes)*1000000.0/double(us):0;
   ++recorded;
   if(completed&&!cancelled&&recorded<total){++step;return true;}
   active=false;done=true;return false;
 }
};
}
