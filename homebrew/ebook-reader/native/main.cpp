// SPDX-License-Identifier: GPL-3.0-or-later
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "ebook_engine.hpp"
#include "usb_bridge.hpp"
#include "comic_archive.hpp"
#include "ebook_layout.hpp"
#include "ebook_reflow.hpp"
#include "ebook_log.hpp"
#include "ebook_cover_store.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_GIF
#define STBI_NO_STDIO
#include "stb_image.h"
#include <algorithm>
#include <cctype>
#include <array>
#include <atomic>
#include <map>
#include <mutex>
#include <pthread.h>
#include <cmath>
#include <cstdint>
#include <limits>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
using namespace hui::gfx;
namespace {
const Color cream=Color::rgb(0xffeac8),gold=Color::rgb(0xf4bb60),muted=Color::rgb(0xc49a6d),
 dark=Color::rgb(0x241306),wood=Color::rgb(0x49250f),edge=Color::rgb(0x875022),paper=Color::rgb(0xfff5dd),ink=Color::rgb(0x422b1c);
void txt(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,float size,Color c=cream,Align align=Align::left) {
 d.text(f,a,s,x,y,size,c,align);
}
std::string cut(std::string s,size_t n=31){if(s.size()>n){s.resize(n-3);s+="...";}return s;}
std::string read(const char* p){
 FILE* f=fopen(p,"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 long n=ftell(f);if(n<1||n>10000000){fclose(f);return {};}
 rewind(f);std::string out((size_t)n,0);
 fread(&out[0],1,out.size(),f);fclose(f);return out;
}

struct TextureImage {std::uint32_t id=0;int width=0,height=0;bool checked=false;};
TextureImage make_image(hui::gfx::Renderer& renderer,const std::string& encoded){
 TextureImage info;info.checked=true;
 if(encoded.empty()||encoded.size()>3*1024*1024)return info;
 int w=0,h=0,n=0;
 if(!stbi_info_from_memory((const stbi_uc*)encoded.data(),(int)encoded.size(),&w,&h,&n)||
    w<1||h<1||w>2048||h>2048||(std::int64_t)w*h>3000000)return info;
 stbi_uc* rgba=stbi_load_from_memory((const stbi_uc*)encoded.data(),(int)encoded.size(),&w,&h,&n,4);
 if(!rgba)return info;
 info.id=renderer.batch().create_texture(w,h,rgba);
 info.width=w;info.height=h;
 stbi_image_free(rgba);return info;
}

// Comic pages are typically larger than EPUB figures. Decode only one page,
// then downscale RGBA to fit within a 2048px texture, retaining aspect ratio.
TextureImage make_comic_texture(hui::gfx::Renderer& renderer,const std::string& encoded){
 TextureImage info;info.checked=true;
 if(encoded.empty()||encoded.size()>20*1024*1024)return info;
 int w=0,h=0,n=0;
 if(!stbi_info_from_memory((const stbi_uc*)encoded.data(),(int)encoded.size(),&w,&h,&n)||
    w<1||h<1||w>7000||h>7000||(std::int64_t)w*h>14000000)return info;
 stbi_uc* raw=stbi_load_from_memory((const stbi_uc*)encoded.data(),(int)encoded.size(),&w,&h,&n,4);
 if(!raw)return info;
 int tw=w,th=h;
 if(w>2048 || h>2048){
    const float scale=std::min(2048.f/w,2048.f/h);
    tw=std::max(1,(int)std::round(w*scale));
    th=std::max(1,(int)std::round(h*scale));
 }
 if(tw==w && th==h){info.id=renderer.batch().create_texture(w,h,raw);}
 else {
   std::vector<unsigned char> down((size_t)tw*(size_t)th*4u);
   for(int y=0;y<th;y++){
      const int sy=std::min(h-1,(int)((y+0.5f)*h/th));
      for(int x=0;x<tw;x++){
       const int sx=std::min(w-1,(int)((x+0.5f)*w/tw));
       std::memcpy(&down[((size_t)y*tw+x)*4u],
                   raw+((size_t)sy*w+sx)*4u,4u);
      }
   }
   info.id=renderer.batch().create_texture(tw,th,down.data());
 }
 info.width=tw;info.height=th;
 stbi_image_free(raw);return info;
}
// Book opening and large USB transfers must NEVER run on the render thread.
// The progress callback is called after actual bytes are written to disk.
struct OpenBookTask {
 ebook::BookEntry entry; bool remote=false;usbbooks::Entry remoteBook;
};
struct OpenBookResult {
 std::string entryPath,localPath,error,cover;
 ebook::Document document;
 comic::Book comic;
 bool ok=false,isComic=false;
};
std::mutex g_openLock;
std::vector<OpenBookResult> g_openReady;
std::atomic<bool> g_openBusy{false},g_openCancel{false};
std::atomic<size_t> g_openDone{0},g_openTotal{0};
std::atomic<int> g_openStage{0}; // 1 copying from USB, 2 parsing pages
bool book_transfer_progress(size_t done,size_t total,void*){
 g_openTotal.store(total);g_openDone.store(done);
 return !g_openCancel.load();
}
void* book_open_worker(void* data){
 auto* task=(OpenBookTask*)data;
 OpenBookResult out;out.entryPath=task->entry.path;
 out.localPath=out.entryPath;
 if(task->remote){
   g_openStage.store(1);
   out.ok=usbbooks::fetch(task->remoteBook,out.localPath,out.error,
                          book_transfer_progress,nullptr);
 }else out.ok=true;
 if(out.ok && !g_openCancel.load()){
   ebook_log::write("OPEN_FILE",out.localPath);
   g_openStage.store(2);
   out.isComic=comic::is_comic(out.localPath);
   if(out.isComic){
     out.ok=comic::open(out.localPath,out.comic);
     out.error=out.comic.error;
     // Cover extraction on the worker; upload the texture on the render
     // thread after successful opening.
     if(out.ok && !out.comic.pages.empty()){
       std::string coverError;
       comic::extract(out.comic,0,out.cover,coverError);
       if(!out.cover.empty())ebook_cover::save(out.entryPath,out.cover);
     }
   }else{
     out.ok=ebook::read_document(out.localPath,out.document);
     out.error=out.document.error;
     out.cover=out.document.coverImage;
   }
 }
 if(g_openCancel.load()){out.ok=false;out.error="Book loading cancelled";}
 if(out.ok)ebook_log::write("OPEN_OK",out.localPath);
 else ebook_log::write("OPEN_ERROR",out.entryPath+" "+out.error);
 {std::lock_guard<std::mutex> lock(g_openLock);
  g_openReady.push_back(std::move(out));}
 delete task;g_openBusy.store(false);return nullptr;
}
bool start_book_open(const ebook::BookEntry& entry,std::string& error){
 if(g_openBusy.exchange(true))return false;
 auto* task=new OpenBookTask;task->entry=entry;
 task->remote=usbbooks::is_remote(entry.path);
 if(task->remote && !usbbooks::entry_from_remote(entry.path,task->remoteBook)){
   delete task;g_openBusy.store(false);
   error="USB catalogue expired. Press OPTIONS to rescan";return false;
 }
 g_openCancel.store(false);g_openDone.store(0);
 g_openTotal.store(task->remote?task->remoteBook.bytes:0);
 g_openStage.store(task->remote?1:2);
 pthread_t t{};
 if(pthread_create(&t,nullptr,book_open_worker,task)!=0){
   delete task;g_openBusy.store(false);
   error="Could not start background book loader";return false;
 }
 pthread_detach(t);return true;
}

struct ComicPageTask {comic::Book book;int page=0;};
struct ComicPageResult {std::string path,bytes,error;int page=0;};
std::atomic<bool> g_comicBusy{false};
std::mutex g_comicLock;
std::vector<ComicPageResult> g_comicReady;
void* comic_page_worker(void* ptr){
 ComicPageTask* task=static_cast<ComicPageTask*>(ptr);
 ComicPageResult result;
 result.page=task->page;
 result.path=task->book.path;
 comic::extract(task->book,(size_t)task->page,result.bytes,result.error);
 {std::lock_guard<std::mutex> lock(g_comicLock);
  g_comicReady.push_back(std::move(result));}
 delete task;
 g_comicBusy.store(false);
 return nullptr;
}
bool start_comic_page(const comic::Book& book,int page){
 if(g_comicBusy.exchange(true))return false;
 auto* task=new ComicPageTask;task->book=book;task->page=page;
 pthread_t thread{};
 if(pthread_create(&thread,nullptr,comic_page_worker,task)!=0){
  delete task;g_comicBusy.store(false);return false;
 }
 pthread_detach(thread);
 return true;
}
void free_texture(TextureImage& t){
 if(t.id){GLuint id=t.id;glDeleteTextures(1,&id);t.id=0;}
 t.checked=false;
}
void draw_fit(DrawList& d,const TextureImage& t,float x,float y,float w,float h){
 if(!t.id||t.width<1||t.height<1)return;
 const float scale=std::min(w/(float)t.width,h/(float)t.height);
 const float fitW=t.width*scale,fitH=t.height*scale;
 d.image(t.id,{x+(w-fitW)*0.5f,y+(h-fitH)*0.5f,fitW,fitH},
         kFullUv,Color::rgb(0xffffff));
}
// Render cover art edge-to-edge inside the physical book border, with a
// centred UV crop. Full cover photos must not look like stuck-on thumbnails.
void draw_cover(DrawList& d,const TextureImage& t,float x,float y,float w,float h){
 if(!t.id||t.width<1||t.height<1)return;
 const auto crop=ebook_layout::cover_uv(t.width,t.height,w,h);
 d.image(t.id,{x,y,w,h},{crop.x,crop.y,crop.w,crop.h},Color::rgb(0xffffff),3);
}
// The file transfer/ZIP extraction runs outside the render thread. OpenGL
// texture creation is always completed on the render thread.
struct CoverTask {
 std::string bookPath;
 bool remote=false;
 bool comic=false;
 usbbooks::Entry remoteBook;
};
struct CoverResult{std::string path,bytes;bool retry=false;};
std::mutex g_coverLock;
std::atomic<bool> g_coverBusy{false};
std::atomic<size_t> g_coverDone{0},g_coverTotal{0};
bool cover_transfer_progress(size_t done,size_t total,void*){
 g_coverDone.store(done);g_coverTotal.store(total);
 return !g_openBusy.load(); // normally true while the cover worker is active
}
std::vector<CoverResult> g_coverReady;
void* cover_worker(void* arg){
 CoverTask* task=(CoverTask*)arg;
 std::string local=task->bookPath,bytes,error;
 bool ok=ebook_cover::load(task->bookPath,bytes);
 if(!ok){
   ok=true;
   // A visible USB comic is fetched once on the worker; thereafter only
   // the small cover is read from /data/EbookReader/Covers.
   if(task->remote)ok=usbbooks::fetch(task->remoteBook,local,error,
                                      cover_transfer_progress,nullptr);
   if(!ok)ebook_log::write("COVER_USB_ERROR",error);
   if(ok){
     if(task->comic)comic::cover(local,bytes);
     else ebook::read_epub_cover(local,bytes);
   }
   if(!bytes.empty())ebook_cover::save(task->bookPath,bytes);
 }
 if(!bytes.empty())ebook_log::write("COVER_READY",task->bookPath);
 {std::lock_guard<std::mutex> lock(g_coverLock);
  g_coverReady.push_back({task->bookPath,std::move(bytes),
                             error=="Book loading cancelled"});
 }
 delete task;
 g_coverBusy.store(false);
 return nullptr;
}
bool start_cover(const ebook::BookEntry& b){
 // Load all visible EPUB/CBR/CBZ covers asynchronously. USB comics can
 // require a full first download; persistent thumbnails avoid repeat work.
 if(g_coverBusy.exchange(true))return false;
 g_coverDone.store(0);g_coverTotal.store(0);
 CoverTask* task=new CoverTask;task->bookPath=b.path;
 task->comic=(b.format=="CBR"||b.format=="CBZ");
 if(usbbooks::is_remote(b.path)){
  task->remote=true;
  if(!usbbooks::entry_from_remote(b.path,task->remoteBook)){
   delete task;g_coverBusy.store(false);return false;
  }
 }
 pthread_t t{};
 if(pthread_create(&t,nullptr,cover_worker,task)!=0){
  delete task;g_coverBusy.store(false);return false;
 }
 pthread_detach(t);return true;
}

void panel(DrawList& d,float x,float y,float w,float h,bool selected=false){
 d.bordered_rect({x,y,w,h},12,selected?Color::rgb(0x5b3416):wood,selected?4:1,selected?gold:edge);
}
void book(DrawList& d,const Font& f,unsigned atlas,const ebook::BookEntry& b,float x,float y,bool selected,const TextureImage& cover){
 constexpr float w=ebook_layout::book_width, h=ebook_layout::book_height;
 d.shadow({x,y,w+6.f,h+7.f},8,12,Color::rgb(0x080301,0.62f));
 if(selected)d.glow({x-3,y-3,w+6,h+6},7,7,gold);
 const unsigned colors[]={0x572b35,0x17454c,0x604c27,0x43355e,0x375032,0x614027};
 unsigned hash=0;for(unsigned char c:b.title)hash=hash*33+c;
 const Color placeholder=Color::rgb(colors[hash%6]);
 // The narrow frame remains visible, but real artwork fills the entire
 // front face. Only books without a cover get a title/format fallback.
 d.bordered_rect({x,y,w,h},5,placeholder,selected?5.f:3.f,selected?gold:Color::rgb(0xb88a4f));
 if(cover.id){
    draw_cover(d,cover,x+5,y+5,w-10.f,h-10.f);
 }else{
    d.line(x+29,y+23,x+w-21,y+23,2,gold);
    d.line(x+29,y+h-27,x+w-21,y+h-27,2,gold);
    txt(d,f,atlas,b.format,x+w-25,y+54,17,gold,Align::right);
    txt(d,f,atlas,cut(b.title,15),x+w/2,y+129,23,cream,Align::center);
    txt(d,f,atlas,cut(b.author,18),x+w/2,y+166,16,muted,Align::center);
 }
 // Small book-spine sheen belongs to the frame, not a label over the cover.
 d.rounded_rect({x+4,y+5,8,h-10.f},0,Color::rgb(0x170b08,0.40f));
 if(b.favorite){
   d.rounded_rect({x+w-33,y+7,24,25},3,Color::rgb(0x201306,0.72f));
   d.star(x+w-21,y+19,9,gold);
 }
 if(b.progress>0){
   d.rounded_rect({x+19,y+h-12,w-38.f,4},2,Color::rgb(0x140d08,0.77f));
   d.rounded_rect({x+19,y+h-12,std::min(w-38.f,(w-38.f)*(float)b.progress/10000.f),4},2,gold);
 }
}
std::vector<std::string> dirs(){return {ebook_log::books_directory(),"/app0/assets/Books","/app0/assets/books","/app0/Books","/download0/Books","/download0/ebooks","/data/ebooks","/mnt/usb0/Books","/mnt/usb1/Books","/mnt/usb0","/mnt/usb1","/mnt/usb2"};}
}
int main(){
 const bool logAvailable=ebook_log::prepare();
 ebook_log::write("BOOT","Ebook Reader v0.2.5 started; Title ID PPSA99741");
 ebook_log::write("DATA","Local Books: "+ebook_log::books_directory()+
                                " Log: "+ebook_log::filename());
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;std::string data=read("/app0/assets/fonts/inter-semibold.huifont");
 if(data.empty())data=read("/data/homebrew/PPSA99741/assets/fonts/inter-semibold.huifont");
 if(!font.load(data))hui::sys::park();
 auto atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 std::vector<ebook::BookEntry> books,imports;
 usbbooks::Catalog usbCatalog;
 auto refresh=[&](){
   ebook_log::write("SCAN_BEGIN","Refreshing local books and USB catalogue");
   // USB bridge's known catalogue is shared with the cover worker.
   if(g_coverBusy.load()||g_openBusy.load()){
    ebook_log::write("SCAN_WAIT","Cover worker still active");
    return;
   }
   ebook::scan_books(dirs(),books);
   usbCatalog=usbbooks::scan();
   for(const auto& item:usbCatalog.books){
     ebook::BookEntry b;
     b.path=usbbooks::remote_path(item);
     b.title=ebook::clean_title(item.relative);
     auto name=item.relative;
     for(char& c:name)c=(char)std::tolower((unsigned char)c);
     b.format=name.size()>=4 && name.substr(name.size()-4)==".pdf"?"PDF":
              name.size()>=4 && name.substr(name.size()-4)==".cbr"?"CBR":
              name.size()>=4 && name.substr(name.size()-4)==".cbz"?"CBZ":"EPUB";
     b.collection=(b.format=="CBR"||b.format=="CBZ")?"Comics":"Fiction";b.author="USB Storage";
     books.push_back(std::move(b));
   }
   ebook::load_state(books);
   std::sort(books.begin(),books.end(),[](const ebook::BookEntry& a,const ebook::BookEntry& b){return a.title<b.title;});
   ebook::scan_imports(imports);
   ebook_log::write("SCAN_DONE","Books="+std::to_string(books.size())+
                        " USB="+std::to_string(usbCatalog.books.size())+
                        " Status="+usbCatalog.message);
 };
 refresh();
 ebook::Document document;
 ebook_reflow::Layout singleText;
 comic::Book currentComic;
 bool inComic=false;
 bool singlePage=false; // Square: EPUB/PDF single vs double-page display
 TextureImage comicPageTexture;
 int comicTexturePage=-1;
 std::string comicImageError;
 float readerZoom=1.f,readerPanX=0.f,readerPanY=0.f;
 std::map<std::string,TextureImage> covers;
 std::map<size_t,TextureImage> pageImages;
 int lastRenderedImagePage=-1;
 int coverTick=0;
 bool reader=false,list=false,confirmDelete=false;
 int selected=0,filter=0,readerPage=0,libraryScroll=0,importSelect=0;
 bool importer=false;std::string message="USB EPUB reader via elfldr 9021 (same bridge as Payload LauncherX).";
 std::string loadingTitle;
 if(!logAvailable)message="Warning: /data/EbookReader not writable";
 const char* names[]={"All","Comics","Fiction","Novels","Favorites"};
 unsigned held=0;int repeatDelay=0;
 for(;;){
  std::array<hui::PadSample,64> samples{};auto n=pad.read(samples);
  unsigned pressed=0,now=held;
  for(size_t i=0;i<n;++i){pressed|=samples[i].buttons&~now;now=samples[i].buttons;}
  held=now;
  if(g_openBusy.load()){
    if(pressed&hui::pad_bits::kCircle)g_openCancel.store(true);
    pressed=0;now=0;
  }
  if(--repeatDelay<=0){
   if(now&hui::pad_bits::kUp)pressed|=hui::pad_bits::kUp;
   if(now&hui::pad_bits::kDown)pressed|=hui::pad_bits::kDown;
   if(now&hui::pad_bits::kLeft)pressed|=hui::pad_bits::kLeft;
   if(now&hui::pad_bits::kRight)pressed|=hui::pad_bits::kRight;
   repeatDelay=11;
  }
  // Upload one completed cover on the OpenGL thread, then schedule the next
  // visible EPUB asynchronously to keep DualSense navigation responsive.
  {
    std::vector<CoverResult> finished;
    {std::lock_guard<std::mutex> lock(g_coverLock);finished.swap(g_coverReady);}
    for(auto& item:finished){
      auto found=covers.find(item.path);
      if(found!=covers.end())free_texture(found->second);
      bool comicCover=false;
      for(const auto& b:books)if(b.path==item.path){comicCover=(b.format=="CBR"||b.format=="CBZ");break;}
      if(!item.retry)covers[item.path]=comicCover?
              make_comic_texture(renderer,item.bytes):make_image(renderer,item.bytes);
   }
   // Limit accumulated artwork to avoid unbounded video-memory growth.
   if(covers.size()>54){
      auto it=covers.begin();
      while(covers.size()>40 && it!=covers.end()){
         free_texture(it->second);it=covers.erase(it);
      }
   }
  }
  // Consume background-completed document on OpenGL rendering thread.
  std::vector<OpenBookResult> opened;
  {std::lock_guard<std::mutex> lock(g_openLock);opened.swap(g_openReady);}
  for(auto& item:opened){
    if(!item.ok){
      message="Cannot open book: "+item.error;
      continue;
    }
    for(auto& pair:pageImages)free_texture(pair.second);
    pageImages.clear();lastRenderedImagePage=-1;
    free_texture(comicPageTexture);comicTexturePage=-1;
    readerZoom=1.f;readerPanX=readerPanY=0.f;
    inComic=item.isComic;
    if(inComic)currentComic=std::move(item.comic);
    else {
      document=std::move(item.document);
      std::vector<size_t> imagePositions;
      for(const auto& im:document.images)imagePositions.push_back(im.line);
      singleText=ebook_reflow::full_width(document.lines,imagePositions);
    }
    if(!item.cover.empty()){
      const auto it=covers.find(item.entryPath);
      if(it==covers.end()||!it->second.id)
        covers[item.entryPath]=inComic?
           make_comic_texture(renderer,item.cover):make_image(renderer,item.cover);
    }
    reader=true;
    const size_t pages=inComic?currentComic.pageCount():document.pageCount();
    const size_t views=inComic?pages:(size_t)ebook_layout::text_page_count(
                  singlePage?singleText.lines.size():document.lines.size(),singlePage);
    size_t previous=0;
    for(const auto& b:books)if(b.path==item.entryPath){previous=b.progress;break;}
    readerPage=std::min<int>(std::max(0,(int)views-1),
               (int)(previous*views/10000));
    message="";
  }
  std::vector<size_t> visible;
  for(size_t i=0;i<books.size();++i){
   bool ok=filter==0||(filter==4?books[i].favorite:books[i].collection==names[filter]);
   if(ok)visible.push_back(i);
  }
  selected=std::clamp(selected,0,std::max(0,(int)visible.size()-1));
  if(pressed&hui::pad_bits::kCircle){
   if(confirmDelete)confirmDelete=false;
   else if(reader){
    reader=false;inComic=false;document=ebook::Document{};
    free_texture(comicPageTexture);
    comicTexturePage=-1;
    readerZoom=1.f;readerPanX=0.f;readerPanY=0.f;
    ebook::save_state(books);
   }
   else if(importer)importer=false;
   else {
    for(auto& item:covers)free_texture(item.second);
    for(auto& item:pageImages)free_texture(item.second);
    free_texture(comicPageTexture);
    pad.close();renderer.release();display.close();hui::sys::quit();
   }
  }
  if(reader){
   const int oldPage=readerPage;
   const bool oldSinglePage=singlePage;
   const float oldZoom=readerZoom;
   if(!inComic && (pressed&hui::pad_bits::kSquare)){
      const size_t current=ebook_layout::first_text_line(readerPage,oldSinglePage);
      const size_t originalLine=oldSinglePage?
           ebook_reflow::source_line(singleText,current):current;
      singlePage=!singlePage;
      const size_t next=singlePage?
           ebook_reflow::reflowed_line(singleText,originalLine):originalLine;
      readerPage=(int)(next/(size_t)ebook_layout::text_lines_per_view(singlePage));
      lastRenderedImagePage=-1;
      for(auto& image:pageImages)free_texture(image.second);
      pageImages.clear();
      readerPanX=readerPanY=0.f;
   }
   const int pageCount=inComic?(int)currentComic.pageCount():
      ebook_layout::text_page_count(singlePage?singleText.lines.size():document.lines.size(),singlePage);
   if(pressed&hui::pad_bits::kR2)readerZoom=std::min(4.f,readerZoom+0.25f);
   if(pressed&hui::pad_bits::kL2)readerZoom=std::max(1.f,readerZoom-0.25f);
   if(pressed&hui::pad_bits::kL3 || pressed&hui::pad_bits::kOptions){
     readerZoom=1.f;readerPanX=readerPanY=0.f;
   }
   if((pressed&hui::pad_bits::kR1 || pressed&hui::pad_bits::kCross)&&readerPage+1<pageCount)++readerPage;
   if((pressed&hui::pad_bits::kL1)&&readerPage>0)--readerPage;
   if(readerZoom>1.01f){
      if(pressed&hui::pad_bits::kLeft) readerPanX+=100.f;
      if(pressed&hui::pad_bits::kRight)readerPanX-=100.f;
      if(pressed&hui::pad_bits::kUp)   readerPanY+=100.f;
      if(pressed&hui::pad_bits::kDown) readerPanY-=100.f;
      // Actual scrolling bounds are computed below, after any page change.
      // The old 340*(zoom-1) clamp could hide the bottom half of tall comics.
   }else {
      readerPanX=readerPanY=0.f;
      if((pressed&hui::pad_bits::kRight)&&readerPage+1<pageCount)++readerPage;
      if((pressed&hui::pad_bits::kLeft)&&readerPage>0)--readerPage;
   }
   if(pressed&hui::pad_bits::kTriangle)readerPage=0;
   // Square switches between one and two pages for EPUB/PDF;
   // comics retain the existing 'last page' shortcut.
   if(inComic && (pressed&hui::pad_bits::kSquare))readerPage=pageCount-1;
   if(oldPage!=readerPage){
      // Retain the zoom while returning to the first line of the new page.
      readerPanX=0.f;
      readerPanY=0.f;
   }
   if(!inComic && singlePage && readerZoom>1.01f){
      if(oldPage!=readerPage || oldSinglePage!=singlePage){
         readerPanX=ebook_layout::single_page_start_x(readerZoom);
      }else if(oldZoom!=readerZoom){
         // Zoom around the text's starting margin rather than the centre
         // of the entire screen.
         readerPanX+=ebook_layout::single_page_start_x(readerZoom)-
                      ebook_layout::single_page_start_x(oldZoom);
      }
   }
   if(readerZoom>1.01f){
      const auto bounds=inComic
         ? ebook_layout::comic_pan_bounds(comicTexturePage==readerPage?comicPageTexture.width:0,
                                         comicTexturePage==readerPage?comicPageTexture.height:0,
                                         readerZoom)
         : ebook_layout::text_pan_bounds(readerZoom,singlePage);
      readerPanX=ebook_layout::clamp_x(readerPanX,bounds);
      readerPanY=ebook_layout::clamp_y(readerPanY,bounds);
   }else{
      readerPanX=readerPanY=0.f;
   }
   if(selected>=0&&selected<(int)visible.size()){
    auto& b=books[visible[selected]];
    b.progress=(size_t)((readerPage*10000ULL)/(size_t)std::max(1,pageCount));
    if(oldPage!=readerPage || oldSinglePage!=singlePage)ebook::save_state(books);
   }
  }else if(importer){
   if(pressed&hui::pad_bits::kTriangle){refresh();importSelect=0;message=usbCatalog.message;}
   if(pressed&hui::pad_bits::kDown)importSelect=std::min((int)imports.size()-1,importSelect+1);
   if(pressed&hui::pad_bits::kUp)importSelect=std::max(0,importSelect-1);
   if((pressed&hui::pad_bits::kCross)&&!imports.empty()){
    ebook::import_to_library(imports[(size_t)importSelect].path,message);
    refresh();selected=0;
   }
  }else{
   if(pressed&hui::pad_bits::kTriangle){importer=true;refresh();importSelect=0;message=usbCatalog.message;}
   if(pressed&hui::pad_bits::kOptions){refresh();selected=0;message="Library: "+std::to_string(books.size())+" books. USB bridge: "+usbCatalog.message;}
   if(pressed&hui::pad_bits::kR1){filter=(filter+1)%5;selected=0;}
   if(pressed&hui::pad_bits::kL1){filter=(filter+4)%5;selected=0;}
   if(pressed&hui::pad_bits::kR2)list=!list;
   int step=list?1:5;
   if(pressed&hui::pad_bits::kUp)selected=std::max(0,selected-step);
   if(pressed&hui::pad_bits::kDown)selected=std::min((int)visible.size()-1,selected+step);
   if(pressed&hui::pad_bits::kLeft)selected=std::max(0,selected-1);
   if(pressed&hui::pad_bits::kRight)selected=std::min((int)visible.size()-1,selected+1);
   if((pressed&hui::pad_bits::kSquare)&&!visible.empty()){
    books[visible[selected]].favorite=!books[visible[selected]].favorite;ebook::save_state(books);
   }
   if((pressed&hui::pad_bits::kCross)&&!visible.empty()){
    if(confirmDelete){
     if(ebook::erase_imported(books[visible[selected]]))message="Book removed from /data/ebooks";
     else message="Only imported library files can be deleted";
     confirmDelete=false;refresh();
    }else{
     const auto& b=books[visible[selected]];
     ebook_log::write("OPEN_REQUEST",b.format+" "+b.path);
     if(start_book_open(b,message)){
       loadingTitle=b.title;
       message="";
     }else if(message.empty())message="Book loader already running";
    }
   }
   if(pressed&hui::pad_bits::kL3)confirmDelete=!confirmDelete;
  }
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x2b1607),Color::rgb(0x120905));
  // Carved wooden frame, parchment-coloured title bar and ornamental lines.
  d.gradient_rect({0,0,1920,92},0,Color::rgb(0x613719),Color::rgb(0x2d1406));
  d.line(0,91,1920,91,5,gold);
  d.bordered_rect({27,17,58,57},11,Color::rgb(0x281406),2,gold);
  d.line(39,34,55,44,4,gold);d.line(55,44,72,34,4,gold);
  d.line(39,34,39,59,3,gold);d.line(72,34,72,59,3,gold);
  txt(d,font,atlas,"EBOOK READER by YAHBALLON",110,57,30,cream);
  txt(d,font,atlas,"v0.2.5",1873,57,22,gold,Align::right);
  if(reader){
   const std::string title=inComic?ebook::clean_title(currentComic.path):document.title;
   const int totalPages=inComic?(int)currentComic.pageCount():
      ebook_layout::text_page_count(singlePage?singleText.lines.size():document.lines.size(),singlePage);
   txt(d,font,atlas,cut(title,62),960,141,28,cream,Align::center);
   d.shadow({167,184,1586,758},19,20,Color::rgb(0x050201,.6f));
   d.bordered_rect({161,180,1598,781},12,Color::rgb(0xb89d75),4,edge);
   // Full-width parchment with no narrow card or unused side gutters.
   d.rounded_rect({177,196,1566,750},5,inComic?Color::rgb(0x101014):paper);
   if(inComic){
     std::vector<ComicPageResult> finished;
     {std::lock_guard<std::mutex> lock(g_comicLock);finished.swap(g_comicReady);}
     for(auto& item:finished){
       if(item.path==currentComic.path && item.page==readerPage){
         free_texture(comicPageTexture);
         comicPageTexture=make_comic_texture(renderer,item.bytes);
         comicTexturePage=item.page;
         comicImageError=item.error;
         if(!comicPageTexture.id&&comicImageError.empty())comicImageError="Image format or resolution unsupported";
       }
     }
     if(comicTexturePage!=readerPage && !g_comicBusy.load()){
       start_comic_page(currentComic,readerPage);
     }
   }
   // A physical-page scissor must be pushed before the zoom transform.
   // Otherwise text overflows onto the dark left/right margins.
   const auto sheetClip=(!inComic && singlePage)
        ? ebook_layout::single_sheet_clip : ebook_layout::page_clip;
   d.push_clip({sheetClip.x,sheetClip.y,sheetClip.w,sheetClip.h});
   // A centre-pivot scales the next page away from the top and forces the
   // user to scroll back up. Anchor EPUB text to row one, and image-based
   // comics to the top of their fitted page inside the reading viewport.
   const float pageTopShift=inComic && comicTexturePage==readerPage
      ? ebook_layout::top_alignment(comicPageTexture.width,comicPageTexture.height,readerZoom)
      : 0.f;
   d.push_transform(readerZoom,960,inComic?ebook_layout::comic_y:264.f,
                    readerPanX,readerPanY+pageTopShift);
   if(inComic){
     if(comicTexturePage==readerPage && comicPageTexture.id)
       draw_fit(d,comicPageTexture,183,200,1554,741);
     else txt(d,font,atlas,comicTexturePage==readerPage?comicImageError:"Loading comic page...",960,554,26,cream,Align::center);
   }else {
     if(!singlePage)d.gradient_rect({952,203,16,728},0,
                                   Color::rgb(0xc9bca1),Color::rgb(0xe8dfc9));
     if(lastRenderedImagePage!=readerPage){
        for(auto& item:pageImages)free_texture(item.second);
        pageImages.clear();
        lastRenderedImagePage=readerPage;
        const size_t first=ebook_layout::first_text_line(readerPage,singlePage);
        const size_t last=first+(size_t)ebook_layout::text_lines_per_view(singlePage);
        for(size_t i=0;i<document.images.size();++i){
          const size_t line=singlePage?singleText.imageLineByIndex[i]:document.images[i].line;
          if(line>=first && line<last)
            pageImages[i]=make_image(renderer,document.images[i].encoded);
        }
     }
     const size_t firstLine=ebook_layout::first_text_line(readerPage,singlePage);
     const int linesPerView=ebook_layout::text_lines_per_view(singlePage);
     const auto& displayLines=singlePage?singleText.lines:document.lines;
     for(int j=0;j<linesPerView;++j){
      const size_t idx=firstLine+(size_t)j;
      if(idx>=displayLines.size())break;
      const int col=singlePage?0:j/17,row=j%17;
      const float x=228.f+col*794.f,y=264.f+row*38.f;
      txt(d,font,atlas,cut(displayLines[idx],singlePage?89:52),x,y,24,ink);
     }
     for(const auto& item:pageImages){
       const auto& img=document.images[item.first];
       const size_t renderLine=singlePage?singleText.imageLineByIndex[item.first]:img.line;
       const size_t offset=renderLine-firstLine;
       const float x=228.f+(singlePage?0.f:(offset/17)*794.f);
       const float y=264.f+(offset%17)*38.f;
       if(item.second.id)draw_fit(d,item.second,x,y,singlePage?1484.f:685.f,240.f);
       else txt(d,font,atlas,"[Unsupported image]",x,y+30.f,17,ink);
     }
   }
   d.pop_transform();
   d.pop_clip();
   // Reading status belongs below the book, never over enlarged text.
   const std::string pageMode=inComic?"COMIC VIEWER - CBR/CBZ":
      (document.isPdfTextPreview?std::string("PDF TEXT PREVIEW"):std::string("EPUB READER"))+
      std::string(singlePage?"  /  SINGLE PAGE":"  /  DOUBLE PAGE");
   txt(d,font,atlas,pageMode,230,997,20,cream);
   txt(d,font,atlas,std::to_string(readerPage+1)+" / "+std::to_string(totalPages),
       960,997,20,gold,Align::center);
   txt(d,font,atlas,std::to_string((int)(readerZoom*100))+"% ZOOM",
       1660,997,20,gold,Align::right);
   txt(d,font,atlas,inComic?
       "L1/R1 Page   L2/R2 Zoom   D-PAD Pan   L3 Reset   SQUARE Last   O Library":
       "L1/R1 Page   L2/R2 Zoom   D-PAD Pan   SQUARE Single/Double   L3 Reset   O Library",
       960,1037,18,cream,Align::center);
  }else if(importer){
   txt(d,font,atlas,"BROWSE STORAGE SOURCES",95,161,36);
   txt(d,font,atlas,"USB may be blocked by the PS5 sandbox. Local app Books can be opened directly.",95,211,19,muted);
   if(imports.empty())txt(d,font,atlas,"No accessible EPUB / PDF / CBR / CBZ found. Check folder status and local Books.",96,312,23,gold);
   int begin=std::max(0,importSelect-9);
   for(int i=begin;i<(int)imports.size()&&i<begin+9;++i){
    float y=249+(i-begin)*74.f;panel(d,92,y,1168,65,i==importSelect);
    txt(d,font,atlas,cut(imports[i].title,49),115,y+38,22);
    txt(d,font,atlas,imports[i].format,1234,y+38,20,gold,Align::right);
   }
   panel(d,1280,249,562,693);
   txt(d,font,atlas,"STORAGE PATH STATUS",1310,295,24,gold);
   const auto& checks=ebook::last_scan_report();
   for(size_t i=0;i<checks.size()&&i<9;i++){
      const auto& check=checks[i];float y=339+(float)i*65;
      txt(d,font,atlas,cut(check.path,36),1308,y,19,cream);
      const std::string result=check.opened?std::to_string(check.books)+" books, "+std::to_string(check.candidates)+" files":("ERROR: "+check.error);
      txt(d,font,atlas,cut(result,47),1310,y+24,15,check.opened?gold:Color::rgb(0xffa785));
   }
   txt(d,font,atlas,"USB helper: "+cut(usbCatalog.message,83),92,964,17,gold);
   txt(d,font,atlas,"X Copy local file    TRIANGLE Rescan bridge    CIRCLE Back",95,1038,21,cream);
  }else{
   const int count=(int)visible.size();
   d.rounded_rect({53,116,1814,92},9,Color::rgb(0x371c0a));
   txt(d,font,atlas,"MY LIBRARY",92,169,42,cream);
   txt(d,font,atlas,std::to_string(books.size())+" books",480,165,24,muted);
   for(int i=0;i<5;++i){
    float x=840+i*200.f;
    panel(d,x,128,184,59,i==filter);
    txt(d,font,atlas,names[i],x+92,166,19,i==filter?gold:cream,Align::center);
   }
   d.rounded_rect({38,233,1845,767},6,Color::rgb(0x2a1607));
   for(int row=0;row<ebook_layout::rows;row++){
    const auto plank=ebook_layout::shelf(row);
    d.gradient_rect({plank.x,plank.y,plank.w,plank.h},0,
                    Color::rgb(0x915829),Color::rgb(0x341808));
    d.line(plank.x,plank.y,plank.x+plank.w,plank.y,3,Color::rgb(0xc48943));
   }
   if(count==0){
      txt(d,font,atlas,"No accessible books in this collection.",960,481,29,muted,Align::center);
      txt(d,font,atlas,"USB bridge: "+cut(usbCatalog.message,70),960,530,22,gold,Align::center);
   }
   if(count>0 && !g_openBusy.load() && !g_coverBusy.load() && ++coverTick%22==0){
     const int perPage=list?9:ebook_layout::per_page;
     const int start=(selected/perPage)*perPage,stop=std::min(count,start+perPage);
     bool started=false;
     // Always show inexpensive EPUB covers before downloading large USB CBRs.
     for(int phase=0;phase<2 && !started;phase++){
       for(int i=start;i<stop;i++){
         const auto& b=books[visible[i]];
         if((phase==0 && b.format!="EPUB") ||
            (phase==1 && b.format!="CBR" && b.format!="CBZ") ||
            covers.find(b.path)!=covers.end())continue;
         if(start_cover(b)){started=true;break;}
         covers[b.path]=TextureImage{0,0,0,true};
       }
     }
   }
   if(count>0){
    int pageSize=list?9:ebook_layout::per_page;
    libraryScroll=selected/pageSize;
    for(int i=libraryScroll*pageSize;i<count&&i<(libraryScroll+1)*pageSize;i++){
     const auto& b=books[visible[i]];
     int index=i-libraryScroll*pageSize;
     if(list){
      float y=260+index*79.f;panel(d,90,y,1715,70,i==selected);
      txt(d,font,atlas,cut(b.title,55),119,y+37,26);
      txt(d,font,atlas,b.format+"  "+b.collection+(b.favorite?"  *":""),1768,y+37,20,gold,Align::right);
     }else{
      const auto pos=ebook_layout::position(index);
      const auto it=covers.find(b.path);
      book(d,font,atlas,b,pos.x,pos.y,i==selected,it==covers.end()?TextureImage{}:it->second);
     }
    }
   }
   txt(d,font,atlas,"COLLECTION: "+std::string(names[filter])+"     "+std::to_string(count)+" books      "+std::to_string(libraryScroll+1)+" / "+std::to_string(std::max(1,(count+(list?8:ebook_layout::per_page-1))/(list?9:ebook_layout::per_page))),90,995,18,muted);
   txt(d,font,atlas,confirmDelete?"DELETE SELECTED BOOK?   X Confirm   CIRCLE Cancel":
    "X Read  L1/R1 Collection  SQUARE Favourite  TRIANGLE Storage  OPTIONS Rescan  R2 View  L3 Delete",88,1044,18,cream);
  }
  if(!message.empty()){d.rounded_rect({410,1010,1100,35},9,Color::rgb(0x49250f));txt(d,font,atlas,cut(message,118),960,1036,17,gold,Align::center);}
  if(g_openBusy.load()){
    const size_t total=g_openTotal.load(),done=g_openDone.load();
    const int stage=g_openStage.load();
    const float fraction=total?std::min(1.f,(float)((double)done/total)):0.f;
    d.rounded_rect({0,100,1920,980},0,Color::rgb(0x080503,0.78f));
    d.shadow({280,353,1360,328},18,24,Color::rgb(0x000000,0.66f));
    d.bordered_rect({280,353,1360,328},18,Color::rgb(0x39200f),2,gold);
    txt(d,font,atlas,stage==1?"DOWNLOADING BOOK FROM USB":"OPENING BOOK",
        960,419,35,cream,Align::center);
    txt(d,font,atlas,cut(loadingTitle,67),960,468,24,muted,Align::center);
    d.rounded_rect({394,530,1132,31},12,Color::rgb(0x130c07));
    d.rounded_rect({397,534,std::max(3.f,1126.f*(stage==1?fraction:1.f)),23},10,gold);
    if(stage==1)
      txt(d,font,atlas,std::to_string((int)(fraction*100))+"%  "+
          std::to_string(done/1048576)+" / "+std::to_string((total+1048575)/1048576)+
          " MiB",960,617,24,cream,Align::center);
    else
      txt(d,font,atlas,"Preparing document and comic pages...",960,617,22,cream,Align::center);
    txt(d,font,atlas,g_openCancel.load()?"Cancelling...":"CIRCLE Cancel",
        960,651,18,muted,Align::center);
  }else if(!reader && g_coverBusy.load() && g_coverTotal.load()>0){
    const size_t total=g_coverTotal.load(),done=g_coverDone.load();
    const float fraction=std::min(1.f,(float)((double)done/total));
    d.rounded_rect({530,909,860,69},9,Color::rgb(0x36200e));
    txt(d,font,atlas,"Downloading comic cover  "+
        std::to_string((int)(fraction*100))+"%  ("+
        std::to_string(done/1048576)+"/"+
        std::to_string((total+1048575)/1048576)+" MiB)",
        960,935,18,cream,Align::center);
    d.rounded_rect({588,948,746,9},4,Color::rgb(0x110905));
    d.rounded_rect({588,948,746.f*fraction,9},4,gold);
  }

  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
