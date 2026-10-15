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
} // namespace ebook_layout
