// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/bible.hpp"
#include "../native/reader_layout.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <sys/stat.h>
int main(){
 const std::string path="/tmp/bible-reader-state-fixture.txt";
 bible::Settings s;
 s.book=42;s.chapter=3;s.selectedVerse=16;s.fontSize=32;
 bible::toggle(s,{42,3,16});
 assert(bible::bookmarked(s,{42,3,16}));
 assert(bible::writeSettings(path,s));
 bible::Settings b;
 assert(bible::readSettings(path,b));
 assert(b.book==42&&b.chapter==3&&b.selectedVerse==16&&b.fontSize==32);
 assert(bible::bookmarked(b,{42,3,16}));
 bible::toggle(b,{42,3,16});assert(b.bookmarks.empty());
 std::remove(path.c_str());
 assert(std::string(bible::kBooks[0])=="Genesis");
 assert(std::string(bible::kBooks[65])=="Revelation");
 bible::Library lib;std::string error;
 assert(!lib.load("/this/path/cannot/exist/bible.tsv",error));
 assert(!error.empty());
 assert(lib.search("love").empty());
 for(int day=1;day<=366;day++){
  auto ref=bible::dailyReference(day);
  assert(ref.book>=0&&ref.book<66&&ref.chapter>=1&&ref.verse>=1);
 }
 // Daily Verse ON preserves the exact old three-column layout.
 const auto compact=bible_ui::layoutFor(true);
 const auto expanded=bible_ui::layoutFor(false);
 assert(compact.showDaily && !expanded.showDaily);
 assert(compact.readerX==469.f && compact.readerWidth==927.f);
 assert(compact.readerX+compact.readerWidth==1396.f);
 assert(expanded.readerX+expanded.readerWidth==1885.f);
 assert(expanded.readerWidth>compact.readerWidth);
 assert(compact.verseTextWidth==730.f);
 assert(expanded.verseTextWidth==1219.f);
 assert(expanded.verseTextWidth>compact.verseTextWidth);
 assert(expanded.readerX>431.f); // No overlap with sidebar.
 std::puts("Bible Reader host tests passed");
}
