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
