// SPDX-License-Identifier: GPL-3.0-or-later
#include "comic_archive.hpp"
#include <archive.h>
#include <archive_entry.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <cstring>
int main(int argc,char** argv){
 assert(argc>=2);
 comic::Book book;
 assert(comic::open(std::string(argv[1])+"/sample.cbz",book));
 assert(book.pages.size()==4);
 assert(book.pages[0]=="Comic/page1.jpg");
 assert(book.pages[1]=="Comic/page2.jpg");
 assert(book.pages[2]=="Comic/page5.jpg");
 assert(book.pages[3]=="Comic/page10.jpg");
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
 // The older smoke test checked ONLY the RAR5 header: a build could
 // pass despite failing to decompress a single CBR comic image. Read the
 // archive's actual compressed file data to EOF for both RAR4 and RAR5.
 for(int i=2;i<argc;i++){
   r=archive_read_new();assert(r);
   assert(archive_read_support_filter_all(r)==ARCHIVE_OK);
   assert(archive_read_support_format_rar(r)==ARCHIVE_OK);
   assert(archive_read_support_format_rar5(r)==ARCHIVE_OK);
   assert(archive_read_open_filename(r,argv[i],10240)==ARCHIVE_OK);
   archive_entry* entry=nullptr;
   int status=ARCHIVE_OK;
   int checked=0;
   while((status=archive_read_next_header(r,&entry))==ARCHIVE_OK){
     if(archive_entry_filetype(entry)==AE_IFREG &&
        archive_entry_size(entry)>0){
       char block[4096];la_ssize_t n;size_t decoded=0;
       while((n=archive_read_data(r,block,sizeof(block)))>0)decoded+=(size_t)n;
       assert(n==0);
       assert(decoded==(size_t)archive_entry_size(entry));
       checked++;
     }else archive_read_data_skip(r);
   }
   assert(status==ARCHIVE_EOF);
   assert(checked>0);
   archive_read_close(r);archive_read_free(r);
   std::printf("RAR payload from %s fully decompressed (%d files)\n",argv[i],checked);
 }
 std::puts("CBZ sorting, image extraction, traversal rejection and CBR decoder smoke test passed");
}
