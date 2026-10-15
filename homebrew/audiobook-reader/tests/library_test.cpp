#include "../native/library.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
int main(){
 const std::string dir=audio::root();
 mkdir(dir.c_str(),0755);mkdir((dir+"/Books").c_str(),0755);
 const std::string file=dir+"/Books/test.M4B";
 FILE* f=fopen(file.c_str(),"wb");assert(f);
 for(int i=0;i<2048;i++)fputc('x',f);fclose(f);
 std::vector<audio::Book> found;audio::scan({dir+"/Books"},found);
 assert(found.size()==1);assert(found[0].title=="test");
 found[0].position=135.2;found[0].duration=3600;found[0].favorite=true;
 assert(audio::save(found));found[0].position=0;found[0].favorite=false;audio::load(found);
 assert(found[0].position>135&&found[0].favorite);remove(file.c_str());
 puts("Audiobook library smoke test passed");
}
