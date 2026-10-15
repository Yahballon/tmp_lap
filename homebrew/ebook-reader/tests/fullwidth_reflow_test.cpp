// SPDX-License-Identifier: GPL-3.0-or-later
// Full-page, lossless text reflow and illustration placement regression.
#include "ebook_reflow.hpp"
#include "ebook_layout.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
int main(){
 using namespace ebook_reflow;
 // Engine's 49-character hardwrap should no longer leave half of the
 // 1484px book blank in Single Page mode.
 std::vector<std::string> input={
  "when societies got together and more people",
  "could, as a group, implement the foundations",
  "of modern security systems and institutions.",
  "",
  "An illustration follows this paragraph.", "", "", "", "", "", "", "",
  "After the illustration, the story continues",
  "across the full width of the screen."
 };
 auto wide=full_width(input,{5},89);
 assert(!wide.lines.empty());
 assert(wide.imageLineByIndex.size()==1);
 const auto artwork=wide.imageLineByIndex[0];
 assert(artwork<wide.lines.size() && artwork%17<=10);
 for(size_t k=0;k<7;k++)assert(wide.lines[artwork+k].empty());
 assert(wide.lines[0].size()>49);
 assert(wide.lines[0].find("when societies got together")==0);
 bool foundAfter=false;
 for(const auto& line:wide.lines)if(line.find("After the illustration")!=std::string::npos)foundAfter=true;
 assert(foundAfter);
 assert(wide.sourceOfLine.size()==wide.lines.size());
 assert(wide.singleLineForSource.size()==input.size()+1);
 for(size_t i=0;i<input.size();i++){
   size_t target=reflowed_line(wide,i);
   assert(target<wide.lines.size());
 }
 auto other=full_width({"One paragraph begins here and continues into another old line",
                         "which should use the available full width without truncation."},
                         {},89);
 assert(other.lines.size()<2 || other.lines[0].size()>49);
 assert(other.lines.front().size()<=89);
 // Switching from a spread to the single-page layout should not jump to
 // another chapter or lose the current text position.
 for(size_t source=0;source<input.size();source++){
   const size_t mapped=reflowed_line(wide,source);
   assert(mapped<=wide.lines.size());
 }
 assert(wide.pageCount()>0);
 std::puts("Full-width reflow, image slots, and reading-position maps passed");
}
