// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/chapter_tracking.hpp"
#include <cassert>
#include <limits>
#include <vector>
#include <cstdio>
int main(){
 using audio::Chapter;
 const std::vector<Chapter> chapters={
  {"Introduction",0,30},{"Chapter Two",30,120},{"Chapter Three",120,300},
  {"Epilogue",300,450}
 };
 assert(audio::chapter_at({},0)==-1);
 assert(audio::chapter_at(chapters,-20)==0);
 assert(audio::chapter_at(chapters,0)==0);
 assert(audio::chapter_at(chapters,29.999)==0);
 assert(audio::chapter_at(chapters,30.0)==1);
 assert(audio::chapter_at(chapters,119.999)==1);
 assert(audio::chapter_at(chapters,120)==2);
 assert(audio::chapter_at(chapters,299)==2);
 assert(audio::chapter_at(chapters,300)==3);
 assert(audio::chapter_at(chapters,1000)==3);
 assert(audio::chapter_at(chapters,120)==2); // forward seek
 assert(audio::chapter_at(chapters,10)==0); // rewind/seek back
 assert(audio::chapter_at(chapters,300)==3); // next-chapter button
 assert(audio::chapter_at(chapters,30)==1); // previous-chapter button
 assert(audio::chapter_at(chapters,std::numeric_limits<double>::quiet_NaN())==0);
 assert(audio::chapter_at(chapters,std::numeric_limits<double>::infinity())==0);
 const std::vector<Chapter> late={{"Late intro",9,25},{"Second",25,90}};
 assert(audio::chapter_at(late,0)==0);
 assert(audio::chapter_at(late,25)==1);
 puts("PASS: live chapter follows audio time, boundaries, forward/back seeking");
}
