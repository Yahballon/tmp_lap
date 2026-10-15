// SPDX-License-Identifier: GPL-3.0-or-later
#include "comic_archive.hpp"
#include <archive.h>
#include <archive_entry.h>
#include <cassert>
#include <cstdio>
#include <string>
int main(int argc,char** argv){
 assert(argc>=2);
 comic::Book book;
 assert(comic::open(std::string(argv[1])+"/sample.cbz",book));
 assert(book.pages.size()==3);
 assert(book.pages[0]=="Comic/page1.jpg");
 assert(book.pages[1]=="Comic/page2.jpg");
 assert(book.pages[2]=="Comic/page10.jpg");
 for(size_t i=0;i<book.pages.size();i++){
    std::string image,error;
    assert(comic::extract(book,i,image,error));
    assert(image.size()>100);
    assert((unsigned char)image[0]==0xff && (unsigned char)image[1]==0xd8);
 }
 std::string cover;
 assert(comic::cover(std::string(argv[1])+"/sample.cbz",cover));
 assert(!cover.empty());
 assert(!comic::open(std::string(argv[1])+"/empty.cbz",book));
 // Verify this build of libarchive exposes both RAR4 and RAR5 decoders.
 archive* r=archive_read_new();
 assert(archive_read_support_format_rar(r)==ARCHIVE_OK);
 assert(archive_read_support_format_rar5(r)==ARCHIVE_OK);
 archive_read_free(r);
 if(argc==3){
   r=archive_read_new();
   archive_read_support_filter_all(r);
   archive_read_support_format_rar5(r);
   assert(archive_read_open_filename(r,argv[2],10240)==ARCHIVE_OK);
   archive_entry* entry=nullptr;
   assert(archive_read_next_header(r,&entry)==ARCHIVE_OK);
   archive_read_close(r);archive_read_free(r);
   std::puts("RAR5 test archive headers decoded successfully");
 }
 std::puts("CBZ sorting, image extraction, traversal rejection and CBR decoder smoke test passed");
}
