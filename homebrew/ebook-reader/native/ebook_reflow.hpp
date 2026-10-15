// SPDX-License-Identifier: GPL-3.0-or-later
// Reflows the existing conservative ~49-column EPUB/PDF text for the
// full-width single-page PS5 viewport without changing double-page layout.
// Image blocks retain their original EPUB-image indices and seven-row height.
#pragma once
#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <limits>
namespace ebook_reflow {
struct Layout {
 std::vector<std::string> lines;
 std::vector<size_t> sourceOfLine;
 std::vector<size_t> singleLineForSource;
 std::vector<size_t> imageLineByIndex;
 size_t pageCount() const {return std::max<size_t>(1,(lines.size()+16)/17);}
};
inline Layout full_width(const std::vector<std::string>& original,
                         const std::vector<size_t>& imagePositions,
                         size_t maxColumns=89){
 Layout out;
 maxColumns=std::clamp<size_t>(maxColumns,50,100);
 out.singleLineForSource.assign(original.size()+1,0);
 out.imageLineByIndex.assign(imagePositions.size(),std::numeric_limits<size_t>::max());
 auto push=[&](std::string line,size_t origin){
  out.lines.push_back(std::move(line));
  out.sourceOfLine.push_back(origin);
 };
 std::string row;
 size_t rowOrigin=0,imageIndex=0;
 auto flush=[&](){
   if(!row.empty()){push(std::move(row),rowOrigin);row.clear();}
 };
 for(size_t i=0;i<original.size();){
   if(imageIndex<imagePositions.size() && i==imagePositions[imageIndex]){
     flush();
     // A seven-row illustration must not spill across consecutive pages.
     while(out.lines.size()%17>10)push("",i);
     out.imageLineByIndex[imageIndex]=out.lines.size();
     for(size_t k=0;k<7 && i+k<original.size();++k){
       out.singleLineForSource[i+k]=out.lines.size();
       push("",i+k);
     }
     i+=std::min<size_t>(7,original.size()-i);
     ++imageIndex;
     continue;
   }
   if(original[i].empty()){
     flush();
     out.singleLineForSource[i]=out.lines.size();
     if(out.lines.empty()||!out.lines.back().empty())push("",i);
     ++i;
     continue;
   }
   std::istringstream words(original[i]);
   std::string word;
   bool firstWord=true;
   size_t firstTarget=out.lines.size();
   while(words>>word){
     if(!row.empty() && row.size()+word.size()+1>maxColumns)flush();
     if(firstWord){firstTarget=out.lines.size();firstWord=false;}
     if(row.empty())rowOrigin=i;
     if(!row.empty())row.push_back(' ');
     row+=word;
   }
   out.singleLineForSource[i]=firstTarget;
   ++i;
 }
 flush();
 out.singleLineForSource[original.size()]=out.lines.size();
 if(out.lines.empty())push("",0);
 return out;
}
inline size_t source_line(const Layout& layout,size_t singleLine){
 if(layout.sourceOfLine.empty())return 0;
 return layout.sourceOfLine[std::min(singleLine,layout.sourceOfLine.size()-1)];
}
inline size_t reflowed_line(const Layout& layout,size_t sourceLine){
 if(layout.singleLineForSource.empty())return 0;
 return layout.singleLineForSource[std::min(sourceLine,layout.singleLineForSource.size()-1)];
}
} // namespace ebook_reflow
