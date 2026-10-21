#include "../native/game.hpp"
#include <cassert>
#include <cstdio>
using namespace wins4;
int main(){
 Game g;g.reset();assert(g.turn==1);
 for(int i=0;i<ROWS;i++){g.turn=1+i%2;assert(g.drop(0));}
 assert(!g.legal(0));assert(!g.drop(0));
 g.reset();for(int i=0;i<3;i++){assert(g.drop(i));assert(g.drop(i));}
 assert(g.drop(3));assert(g.winner==1);
 g.reset();for(int i=0;i<3;i++){g.drop(0);g.drop(1);}g.drop(0);assert(g.winner==1);
 g.reset();for(int i=0;i<3;i++){g.drop(i);g.drop(5+i%2);}
 g.level=Level::Hard;assert(g.choose_bot()==3);
 g.reset();g.level=Level::Hard;g.drop(3);g.drop(3);g.drop(2);g.drop(4);
 auto board=g.grid;auto heights=g.heights;auto moves=g.moves;
 assert(g.choose_bot()>=0);assert(g.grid==board&&g.heights==heights&&g.moves==moves);
 puts("PASS: gravity, horizontal/vertical wins, AI and search integrity");
}
