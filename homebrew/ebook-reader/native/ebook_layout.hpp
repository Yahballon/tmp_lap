// SPDX-License-Identifier: GPL-3.0-or-later
// Pure 1920x1080 bookshelf/reading geometry, shared by UI and CI tests.
#pragma once
#include <algorithm>
#include <cmath>
namespace ebook_layout {
// Two well-spaced 5-book rows fit within the shelf viewport; never let a
// third row overlap the book in front or cover the bottom controller hints.
constexpr int columns=5;
constexpr int rows=2;
constexpr int per_page=columns*rows;
constexpr float book_width=208.f;
constexpr float book_height=268.f;
constexpr float first_x=151.f, column_pitch=348.f;
constexpr float first_y=266.f, row_pitch=333.f;
constexpr float shelf_y=529.f, shelf_pitch=333.f;
struct Rectangle{float x=0,y=0,w=0,h=0;};
inline Rectangle position(int index){
 return {first_x+(index%columns)*column_pitch,
         first_y+(index/columns)*row_pitch,book_width,book_height};
}
inline Rectangle shelf(int row){return {65.f,shelf_y+row*shelf_pitch,1783.f,32.f};}
// Return normalized UVs for a centre-cropped image that fills a book cover.
// Never stretch or letterbox the front artwork.
inline Rectangle cover_uv(int width,int height,float w,float h){
 if(width<=0||height<=0||w<=0||h<=0)return {0,0,1,1};
 const float ratio=(float)width/height;
 const float dest=w/h;
 if(ratio>dest){const float fraction=dest/ratio;return {(1.f-fraction)*.5f,0.f,fraction,1.f};}
 const float fraction=ratio/dest;
 return {0.f,(1.f-fraction)*.5f,1.f,fraction};
}
constexpr float comic_x=183.f,comic_y=200.f,comic_w=1554.f,comic_h=741.f;
// When zoomed, align the TOP of the new page with the top of the clipped
// viewport even for landscape comics whose "fit" image is letterboxed.
inline float top_alignment(int tex_w,int tex_h,float zoom){
 if(zoom<=1.f||tex_w<=0||tex_h<=0)return 0.f;
 const float scale=std::min(comic_w/tex_w,comic_h/tex_h);
 const float rendered_h=tex_h*scale;
 const float gap=(comic_h-rendered_h)*.5f;
 return -zoom*gap;
}

// Visible comic/text content must reach BOTH ends of the viewport after
// zooming. Fixed clamps based on (zoom-1) prevented reaching the last part of
// tall pages at 200–400% zoom.
struct PanBounds {float min_x=0.f,max_x=0.f,min_y=0.f,max_y=0.f;};
constexpr Rectangle page_clip{179.f,199.f,1562.f,744.f};
constexpr Rectangle text_page_bounds{218.f,264.f,1484.f,650.f};
// True text extents (first glyph at 550) and the inset of the visible
// cream-coloured sheet. The viewport must be narrower than the whole book,
// otherwise zoomed text bleeds into the brown margins in Single Page mode.
constexpr Rectangle single_text_page_bounds{550.f,264.f,835.f,650.f};
constexpr Rectangle single_sheet_clip{474.f,204.f,972.f,738.f};
constexpr float single_first_glyph_x=550.f;
// Legacy files store lines independent of the reader's display mode:
// a spread shows 34 text lines (2 x 17), a single page shows 17.
constexpr int text_rows=17;
inline int text_lines_per_view(bool single){return single?text_rows:text_rows*2;}
inline int text_page_count(size_t lines,bool single){
 const size_t per=(size_t)text_lines_per_view(single);
 return (int)std::max<size_t>(1,lines/per+(lines%per!=0));
}
inline size_t first_text_line(int page,bool single){
 return (size_t)std::max(0,page)*(size_t)text_lines_per_view(single);
}
inline int remap_text_page(int page,bool was_single,bool to_single){
 const size_t line=first_text_line(page,was_single);
 return (int)(line/(size_t)text_lines_per_view(to_single));
}

inline Rectangle comic_image_bounds(int tex_w,int tex_h){
 if(tex_w<=0||tex_h<=0)return {comic_x,comic_y,comic_w,comic_h};
 const float scale=std::min(comic_w/tex_w,comic_h/tex_h);
 const float w=tex_w*scale,h=tex_h*scale;
 return {comic_x+(comic_w-w)*.5f,comic_y+(comic_h-h)*.5f,w,h};
}
inline PanBounds calculate_pan(Rectangle content,float zoom,float origin_x,float origin_y,
                               float added_y,Rectangle viewport=page_clip){
 PanBounds bounds;
 if(zoom<=1.001f || content.w<=0.f || content.h<=0.f)return bounds;
 const float left=origin_x+(content.x-origin_x)*zoom;
 const float right=left+content.w*zoom;
 const float top=origin_y+(content.y-origin_y)*zoom+added_y;
 const float bottom=top+content.h*zoom;
 if(content.w*zoom>viewport.w+0.5f){
   bounds.min_x=viewport.x+viewport.w-right;
   bounds.max_x=viewport.x-left;
   // Preserve centred/focused default when there is spare space.
   bounds.min_x=std::min(0.f,bounds.min_x);
   bounds.max_x=std::max(0.f,bounds.max_x);
 }
 if(content.h*zoom>viewport.h+0.5f){
   // Zero always means the beginning of a page; the negative bound
   // brings its actual bottom into view, regardless of magnification.
   bounds.min_y=std::min(0.f,viewport.y+viewport.h-bottom);
   bounds.max_y=0.f;
 }
 return bounds;
}
inline PanBounds comic_pan_bounds(int w,int h,float zoom){
 if(w<=0||h<=0)return {};
 const auto content=comic_image_bounds(w,h);
 const float shift=top_alignment(w,h,zoom);
 return calculate_pan(content,zoom,960.f,comic_y,shift);
}
inline PanBounds text_pan_bounds(float zoom,bool single=false){
 if(!single)return calculate_pan(text_page_bounds,zoom,960.f,264.f,0.f);
 auto bounds=calculate_pan(single_text_page_bounds,zoom,960.f,264.f,0.f,
                           single_sheet_clip);
 if(zoom>1.001f){
   // At zoom>100%, x=0 would hide the beginning of every line to the left
   // of the physical paper (because our transform pivots at x=960).
   const float zoomed_first=960.f+(single_first_glyph_x-960.f)*zoom;
   bounds.max_x=single_first_glyph_x-zoomed_first;
   // Scrolling right must still reveal the last part of each line.
   bounds.min_x=std::min(bounds.min_x,bounds.max_x);
 }
 return bounds;
}
inline float single_page_start_x(float zoom){
 return text_pan_bounds(zoom,true).max_x;
}
inline float clamp_x(float pan,const PanBounds& bounds){
 return std::clamp(pan,bounds.min_x,bounds.max_x);
}
inline float clamp_y(float pan,const PanBounds& bounds){
 return std::clamp(pan,bounds.min_y,bounds.max_y);
}
} // namespace ebook_layout
