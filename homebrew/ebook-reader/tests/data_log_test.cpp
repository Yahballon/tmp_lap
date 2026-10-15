// SPDX-License-Identifier: GPL-3.0-or-later
// Host regression: local PS5 data directory, writable log and mixed-format books.
#include "ebook_engine.hpp"
#include "ebook_log.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
int main(){
 assert(ebook_log::prepare());
 assert(ebook_log::root().find("/tmp/") == 0);
 const std::string books=ebook_log::books_directory();
 for(const char* filename:{"Standalone.EPUB","Comic.CBR","More.CBZ","Manual.PDF"}){
   auto path=books+"/"+filename;
   FILE* f=std::fopen(path.c_str(),"wb");
   assert(f);assert(std::fwrite("BOOK",1,4,f)==4);assert(std::fclose(f)==0);
 }
 std::vector<ebook::BookEntry> results;
 assert(ebook::scan_books({books},results));
 assert(results.size()==4);
 int comics=0;
 for(const auto& entry:results)if(entry.collection=="Comics")comics++;
 assert(comics==2);
 // The import view should also find these local books.
 assert(ebook::scan_imports(results));
 assert(results.size()>=4);
 ebook_log::write("BOOK_OPEN","Manual.PDF");
 ebook_log::write("USB_ERROR","Transfer failed: ENOSPC");
 ebook_log::write("SANITIZE","one\nline\ronly");
 FILE* log=std::fopen(ebook_log::filename().c_str(),"rb");
 assert(log);
 char text[4096]={0};
 const size_t got=std::fread(text,1,sizeof(text)-1,log);
 assert(std::fclose(log)==0);
 const std::string contents(text,got);
 assert(contents.find("BOOK_OPEN")!=std::string::npos);
 assert(contents.find("USB_ERROR")!=std::string::npos);
 assert(contents.find("one line only")!=std::string::npos);
 assert(contents.find("one\nline")==std::string::npos);
 std::puts("Local PS5 books, diagnostics and format scan passed");
}
