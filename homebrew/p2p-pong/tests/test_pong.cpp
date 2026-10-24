#include "../core/pong.hpp"
#include <cassert>
int main(){pong::State s;for(int i=0;i<600;i++)pong::step(s,1.f/60.f,0.f,0.f);assert(s.score_l+s.score_r>0);assert(s.left>=.105f&&s.left<=.895f);assert(s.right>=.105f&&s.right<=.895f);pong::reset(s,-1);assert(s.vx<0);}