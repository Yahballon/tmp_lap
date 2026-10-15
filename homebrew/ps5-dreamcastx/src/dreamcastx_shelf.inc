/*
 * PS5DreamcastX - original DVD-style game shelf.
 * Copyright (C) 2026 YAHBALLON contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * All effects are bounded to the case. No projected quads / fake floor
 * reflections; covers retain the original aspect ratio without cropping.
 */
struct DreamcastXArtFit { float x,y,w,h; };
static DreamcastXArtFit dreamcastXFit(float x,float y,float w,float h,float aspect)
{
    if (!std::isfinite(aspect) || aspect<=0.f) aspect=1.f;
    aspect=std::clamp(aspect,0.35f,2.8f);
    if (w<=0.f || h<=0.f) return {x,y,0.f,0.f};
    const float fw=std::min(w,h*aspect);
    const float fh=fw/aspect;
    return {x+(w-fw)*0.5f,y+(h-fh)*0.5f,fw,fh};
}
static void drawDreamcastXCase(ImDrawList *dl,Game& g,
                               float x,float y,float w,float h,bool selected)
{
    const float edge=std::max(5.f,w*0.025f);
    const float spine=std::max(9.f,w*0.060f);
    const float header=std::max(20.f,h*0.068f);
    const float radius=std::max(5.f,w*0.030f);
    // Small contact shadow, not a projected polygon.
    rect(dl,x+4.f,y+7.f,w,h,col::rgba(0,0,0,80),radius);
    if (selected) rect(dl,x-4.f,y-4.f,w+8.f,h+8.f,alpha(col::accent,0.12f),radius+3.f);
    rect(dl,x,y,w,h,col::rgba(34,54,78,244),radius);
    rect(dl,x+spine,y+3.f,w-spine-3.f,h-6.f,col::rgba(11,23,39,255),radius-2.f);
    rect(dl,x+2.f,y+4.f,spine-2.f,h-8.f,col::rgba(91,126,160,150),2.f);
    dl->AddLine(V(x+spine,y+5.f),V(x+spine,y+h-5.f),
                col::rgba(206,229,255,115),1.f*S);
    const float topSize=std::max(12.f,header*0.58f);
    text(dl,bold(),topSize,x+spine+edge*1.6f,y+header*0.20f,
         selected?col::text:col::dim,"DREAMCAST");
    const float ax=x+spine+edge;
    const float ay=y+header+edge*0.40f;
    const float aw=w-spine-edge*2.f;
    const float ah=h-header-edge*1.85f;
    rect(dl,ax,ay,aw,ah,col::rgba(9,17,30,255),2.f);
    const ImTextureID art=coverTexture(g);
    if (art!=ImTextureID())
    {
        const float ratio=imguiDriver!=nullptr?imguiDriver->getAspectRatio(art):1.f;
        const DreamcastXArtFit fitted=dreamcastXFit(ax,ay,aw,ah,ratio);
        // imageFill crops nothing when its destination matches image ratio.
        imageFill(dl,art,fitted.x,fitted.y,fitted.w,fitted.h,2.f);
    }
    else
        placeholder(dl,g,ax,ay,aw,ah,2.f);
    dl->AddLine(V(x+spine+2.f,y+header),V(x+w-5.f,y+header),
                col::rgba(205,228,249,50),1.f*S);
    outline(dl,x,y,w,h,selected?alpha(col::accent,0.95f):
            col::rgba(151,184,214,155),radius,selected?3.2f:1.2f);
}
void drawDreamcastXShelf(ImDrawList *dl,Game *focused,bool selectDisk)
{
    if (lib.shelves.empty()) return;
    Shelf& sh=lib.shelves[std::clamp(lib.shelf,0,(int)lib.shelves.size()-1)];
    sh.focusAnim=motion()==MotionOff||sh.focusAnim<0?
                 (float)sh.focus:approach(sh.focusAnim,(float)sh.focus,13.f);
    const float cx=W*0.5f, cy=479.f;
    const int n=(int)sh.items.size();
    const int start=std::max(0,(int)std::floor(sh.focusAnim)-6);
    const int end=std::min(n-1,(int)std::ceil(sh.focusAnim)+6);
    std::vector<int> order;
    for (int i=start;i<=end;i++) order.push_back(i);
    std::sort(order.begin(),order.end(),[&](int a,int b){
        return std::fabs(a-sh.focusAnim)>std::fabs(b-sh.focusAnim);
    });
    dl->PushClipRect(V(38.f,173.f),V(W-38.f,H-175.f),true);
    for (int i:order)
    {
        Game& g=games[sh.items[(size_t)i]];
        const float delta=(float)i-sh.focusAnim;
        const float distance=std::fabs(delta);
        const float scale=std::max(0.55f,1.f-0.20f*distance);
        const float w=262.f*scale,h=375.f*scale;
        const float x=cx+delta*260.f-w*0.5f;
        const float y=cy+std::min(distance,3.f)*16.f-h*0.5f;
        const bool selected=i==sh.focus&&distance<0.52f&&!lib.details;
        drawDreamcastXCase(dl,g,x,y,w,h,selected);
    }
    dl->PopClipRect();
    const std::string label=(selectDisk?"CHOOSE DISC":"DREAMCAST LIBRARY")
        +std::string("   /   ")+sh.name;
    text(dl,bold(),26,96.f,148.f,col::dim,label.c_str());
    const std::string count=std::to_string(n?sh.focus+1:0)+" / "+std::to_string(n);
    const ImVec2 ct=textSize(regular(),23,count.c_str());
    text(dl,regular(),23,W-96-ct.x,148.f,col::dim,count.c_str());
    if (focused)
    {
        const std::string title=fit(bold(),45,focused->title,W-240.f);
        const ImVec2 ts=textSize(bold(),45,title.c_str());
        text(dl,bold(),45,(W-ts.x)*0.5f,851.f,col::text,title.c_str());
        const std::string meta=metaText(*focused);
        const ImVec2 ms=textSize(regular(),23,meta.c_str());
        text(dl,regular(),23,(W-ms.x)*0.5f,910.f,col::dim,meta.c_str());
    }
}
