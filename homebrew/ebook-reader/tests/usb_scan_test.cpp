// SPDX-License-Identifier: GPL-3.0-or-later
// Host-side regression tests for USB-style nested folders and extension matching.
#include "ebook_engine.hpp"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
int main() {
 char base[]="/tmp/ps5-ebook-scan-XXXXXX";
 const char* temp=mkdtemp(base);
 assert(temp);
 const std::string root=temp;
 const std::string books=root+"/Books";
 const std::string authors=books+"/Authors";
 assert(mkdir(books.c_str(),0700)==0);
 assert(mkdir(authors.c_str(),0700)==0);
 const std::string first=books+"/Novel.EPUB";
 const std::string second=authors+"/Comic.PdF";
 const std::string other=books+"/ignored.txt";
 for(const auto& path: {first,second,other}) {
   FILE* f=fopen(path.c_str(),"wb");assert(f);
   assert(fwrite("x",1,1,f)==1);assert(fclose(f)==0);
 }
 std::vector<ebook::BookEntry> items;
 assert(ebook::scan_books({root},items));
 assert(items.size()==2);
 assert(items[0].format=="PDF"||items[0].format=="EPUB");
 assert(ebook::last_scan_report().size()==1);
 const auto& report=ebook::last_scan_report().front();
 assert(report.opened && report.books==2 && report.candidates==2);
 assert(ebook::scan_books({books,root},items));
 assert(items.size()==2); // duplicate path scanned twice, deduplicated
 assert(ebook::scan_books({root+"/missing"},items));
 assert(items.empty());
 assert(!ebook::last_scan_report().front().opened);
 assert(!ebook::last_scan_report().front().error.empty());
 for(const auto& f:{first,second,other})assert(unlink(f.c_str())==0);
 assert(rmdir(authors.c_str())==0);
 assert(rmdir(books.c_str())==0);
 assert(rmdir(root.c_str())==0);
 std::puts("USB EPUB scanner regression tests passed");
}
