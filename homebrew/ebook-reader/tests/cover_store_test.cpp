// SPDX-License-Identifier: GPL-3.0-or-later
#include "ebook_cover_store.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <unistd.h>
int main(){
 assert(ebook_log::prepare());
 const std::string k="usbbooks:sample comic (2026).cbr";
 const std::string jpeg("\xff\xd8\xff\xe0JPEG-COVER-IMAGE",20);
 const std::string png("\x89PNG\r\n\x1a\nIMAGE-DATA",18);
 const std::string unknown="this is not a cover";
 assert(!ebook_cover::image(unknown));
 assert(ebook_cover::image(jpeg));
 assert(ebook_cover::image(png));
 assert(ebook_cover::save(k,jpeg));
 assert(ebook_cover::path(k).find("/Covers/cover-")!=std::string::npos);
 std::string result;
 assert(ebook_cover::load(k,result) && result==jpeg);
 assert(!ebook_cover::load(k+".different",result));
 assert(ebook_cover::save(k,png));
 assert(ebook_cover::load(k,result) && result==png);
 assert(!ebook_cover::save(k,unknown));
 assert(ebook_cover::load(k,result) && result==png);
 std::puts("Persistent USB comic cover read/write, replacement and validation passed");
}
