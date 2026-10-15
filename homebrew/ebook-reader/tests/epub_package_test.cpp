// SPDX-License-Identifier: GPL-3.0-or-later
// Host EPUB 2/3 container regression test for the rootfiles/rootfile mix-up.
#include "ebook_engine.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
int main(int argc,char **argv){
 assert(argc==2);
 const std::string dir=argv[1];
 for(const char* test:{"standard","namespaced","with_comment","multi_root"}){
  ebook::Document d;
  const std::string file=dir+"/"+test+".epub";
  if(!ebook::read_document(file,d)){
   std::fprintf(stderr,"%s: unexpectedly failed: %s\n",test,d.error.c_str());
   return 1;
  }
  assert(d.title=="Example Test Book");
  assert(d.author=="Test Author");
  assert(!d.lines.empty());
  bool found=false;
  for(const auto& line:d.lines)if(line.find("Hello from the example EPUB")!=std::string::npos)found=true;
  assert(found);
  assert(d.pageCount()>=1);
  std::printf("%s: EPUB package and chapter loaded\n",test);
 }
 for(const char* kind:{"with_images_epub2","with_images_epub3"}){
  ebook::Document d;const std::string file=dir+"/"+kind+".epub";
  assert(ebook::read_document(file,d));
  assert(d.coverImage.size()>4);
  assert((unsigned char)d.coverImage[0]==0xff && (unsigned char)d.coverImage[1]==0xd8);
  assert(d.images.size()==1);
  assert(d.images[0].encoded.size()>8);
  assert(d.images[0].encoded.compare(1,3,"PNG")==0);
  assert(d.images[0].line<d.lines.size());
  assert(d.images[0].line%17<=10);
  assert(d.images[0].alt=="Scene inside book");
  std::string cover;
  assert(ebook::read_epub_cover(file,cover)&&cover==d.coverImage);
  bool before=false,after=false;
  for(const auto& line:d.lines){
    if(line.find("Before the illustration")!=std::string::npos)before=true;
    if(line.find("After the illustration")!=std::string::npos)after=true;
  }
  assert(before&&after);
  std::printf("%s: EPUB JPEG cover and inline PNG extracted\n",kind);
 }
 for(const char* bad:{"traversal","missing_root"}){
  ebook::Document d;
  if(ebook::read_document(dir+"/"+bad+".epub",d)){
   std::fprintf(stderr,"%s: unexpectedly accepted\n",bad);
   return 1;
  }
  assert(!d.error.empty());
 }
 std::puts("EPUB package regression checks passed");
}
