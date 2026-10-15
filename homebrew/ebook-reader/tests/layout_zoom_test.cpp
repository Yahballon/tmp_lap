// SPDX-License-Identifier: GPL-3.0-or-later
// Host checks for layout regressions seen on a physical PS5.
#include "ebook_layout.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <utility>
static bool close(float a,float b){return std::fabs(a-b)<0.01f;}
int main(){
 using namespace ebook_layout;
 assert(columns==5 && rows==2 && per_page==10);
 for(int i=0;i<per_page;i++){
   const auto p=position(i);
   assert(p.x>=65.f && p.x+p.w<1848.f);
   assert(p.y>=233.f && p.y+p.h<965.f);
   for(int j=0;j<i;j++){
      const auto q=position(j);
      // No two book rectangles may touch/overlap, including different rows.
      assert(p.x>=q.x+q.w+15.f || q.x>=p.x+p.w+15.f ||
             p.y>=q.y+q.h+15.f || q.y>=p.y+p.h+15.f);
   }
 }
 assert(shelf(0).y+32.f<position(5).y);
 assert(position(9).y+position(9).h<965.f);
 // The crop fills both portrait and landscape art without stretching.
 const auto tall=cover_uv(600,1000,198,258);
 assert(close(tall.w,1.f) && tall.h<1.f && tall.h>0.f);
 assert(close(tall.y,(1.f-tall.h)/2.f));
 const auto wide=cover_uv(1200,700,198,258);
 assert(wide.w<1.f && close(wide.h,1.f));
 assert(close(wide.x,(1.f-wide.w)/2.f));
 const auto equal=cover_uv(600,600,200,200);
 assert(close(equal.x,0.f) && close(equal.y,0.f) && close(equal.w,1.f));
 // When scaled about clip-top y=200 and offset by top_alignment(),
 // the fitted image top remains exactly at y=200 after every page change.
 for(float zoom: {1.25f,1.5f,2.f,3.f,4.f}){
    for(auto size: {std::pair<int,int>{1000,2000},{1600,900},{1400,1400}}){
      const float fitted=std::min(comic_w/size.first,comic_h/size.second)*size.second;
      const float fittedTop=comic_y+(comic_h-fitted)*.5f;
      const float transformed=comic_y+(fittedTop-comic_y)*zoom
                                      +top_alignment(size.first,size.second,zoom);
      assert(close(transformed,comic_y));
    }
 }
 assert(close(top_alignment(1600,900,1.f),0.f));
 // Regression: at 200–400% zoom, the LAST pixel row must be reachable
 // by pressing Down, for both tall and very wide CBR/CBZ pages.
 for(float zoom: {1.f,1.25f,1.5f,2.f,2.5f,3.f,3.5f,4.f}){
   for(const auto& dims: {std::pair<int,int>{600,2048},{1100,1700},
                          {1600,900},{2000,450},{1400,1400}}){
     const auto image=comic_image_bounds(dims.first,dims.second);
     const float shift=top_alignment(dims.first,dims.second,zoom);
     const auto bounds=comic_pan_bounds(dims.first,dims.second,zoom);
     assert(bounds.min_y<=bounds.max_y);
     assert(bounds.min_x<=bounds.max_x);
     assert(close(clamp_y(-99999.f,bounds),bounds.min_y));
     assert(close(clamp_y(99999.f,bounds),bounds.max_y));
     if(zoom>1.001f && image.h*zoom>page_clip.h+0.5f){
       const float transformedBottom=comic_y+(image.y+image.h-comic_y)*zoom+
                                      shift+bounds.min_y;
       const float clipBottom=page_clip.y+page_clip.h;
       assert(close(transformedBottom,clipBottom));
       const float transformedTop=comic_y+(image.y-comic_y)*zoom+shift+
                                   bounds.max_y;
       assert(transformedTop>=page_clip.y-1.f &&
              transformedTop<=page_clip.y+2.f);
     }else{
       assert(close(bounds.min_y,0.f));
       assert(close(bounds.max_y,0.f));
     }
     if(zoom>1.001f && image.w*zoom>page_clip.w+0.5f){
       const float transformedRight=960.f+(image.x+image.w-960.f)*zoom+bounds.min_x;
       const float transformedLeft=960.f+(image.x-960.f)*zoom+bounds.max_x;
       assert(close(transformedRight,page_clip.x+page_clip.w));
       assert(close(transformedLeft,page_clip.x));
     }
   }
   const auto textBounds=text_pan_bounds(zoom);
   if(zoom>1.001f && text_page_bounds.h*zoom>page_clip.h+0.5f){
     const float bottom=264.f+(text_page_bounds.y+text_page_bounds.h-264.f)*zoom+
                        textBounds.min_y;
     assert(close(bottom,page_clip.y+page_clip.h));
   }
   assert(close(textBounds.max_y,0.f)); // Page changes start at top.
 }

 // Single-page mode displays 17 rows, double-page shows two columns of 17.
 // Switching must preserve the first visible line of the current spread.
 for(size_t lines: {size_t(0),size_t(1),size_t(17),size_t(18),
                    size_t(34),size_t(35),size_t(101)}){
   const int singles=text_page_count(lines,true),doubles=text_page_count(lines,false);
   assert(singles==std::max(1,(int)((lines+16)/17)));
   assert(doubles==std::max(1,(int)((lines+33)/34)));
   for(int page=0;page<doubles;page++){
      const int single=remap_text_page(page,false,true);
      assert(single==page*2);
      assert(remap_text_page(single,true,false)==page);
      assert(first_text_line(page,false)==first_text_line(single,true));
   }
   for(int page=0;page<singles;page++){
      const int spread=remap_text_page(page,true,false);
      assert(spread==page/2);
      assert(first_text_line(spread,false)<=first_text_line(page,true));
      assert(first_text_line(page,true)-first_text_line(spread,false)<34);
   }
 }
 for(float zoom:{1.f,1.25f,2.f,3.f,4.f}){
   const auto bounds=text_pan_bounds(zoom,true);
   assert(bounds.min_y<=0.f && close(bounds.max_y,0.f));
   if(zoom>1.001f && single_text_page_bounds.h*zoom>page_clip.h+0.5f){
      const float imageBottom=264.f+(single_text_page_bounds.y+single_text_page_bounds.h-264.f)*zoom+
                              bounds.min_y;
      assert(close(imageBottom,page_clip.y+page_clip.h));
   }
 }
 std::puts("Book cover spacing, zoom-to-bottom and Square single/double page toggle passed");

}
