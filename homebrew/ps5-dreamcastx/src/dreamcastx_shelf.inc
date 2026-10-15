/*
 * PS5DreamcastX - PS5GBAX-inspired cover-flow renderer.
 * Copyright (C) 2026 YAHBALLON contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Included within the existing PSFlycast ImGui/Vulkan presentation layer.
 * Flycast's real 3D game renderer remains unchanged.
 */
void drawDreamcastXShelf(ImDrawList *dl, Game *focused, bool selectDisk)
{
    if (lib.shelves.empty()) return;
    Shelf& sh = lib.shelves[std::clamp(lib.shelf,0,(int)lib.shelves.size()-1)];
    const bool moving = motion()!=MotionOff;
    sh.focusAnim = !moving || sh.focusAnim<0
        ? (float)sh.focus : approach(sh.focusAnim,(float)sh.focus,13.f);
    const float cx=W*0.5f, cy=486.f, floor=781.f;
    softDisc(dl,cx,floor-24,570.f,alpha(col::accent,0.12f),48);
    gradientV(dl,0,floor-86.f,W,166.f,
        col::rgba(12,23,45,0),col::rgba(6,12,27,167));
    dl->AddLine(V(48.f,floor),V(W-48.f,floor),alpha(col::accent,0.32f),2.f*S);

    // Render a 15-case window back-to-front, without scanning every ROM.
    const int n=(int)sh.items.size();
    const int start=std::max(0,(int)std::floor(sh.focusAnim)-7);
    const int end=std::min(n-1,(int)std::ceil(sh.focusAnim)+7);
    std::vector<int> order;
    for(int i=start;i<=end;i++)order.push_back(i);
    std::sort(order.begin(),order.end(),[&](int a,int b){
        return std::fabs(a-sh.focusAnim)>std::fabs(b-sh.focusAnim);
    });
    dl->PushClipRect(V(22,158),V(W-22,H-126),true);
    for(int i:order)
    {
        Game& g=games[sh.items[(size_t)i]];
        const float delta=(float)i-sh.focusAnim;
        const float dist=std::fabs(delta);
        const float spread=std::clamp(dist/0.86f,0.f,1.f);
        const float sign=delta<0?-1.f:1.f;
        const float x=cx+sign*(300.f+std::max(0.f,dist-1.f)*87.f)*spread;
        const float scale=std::max(0.49f,1.f-0.26f*spread-0.05f*std::max(0.f,dist-1.f));
        const float w=300.f*scale,h=430.f*scale;
        const float y=cy+27.f*spread;
        const float skew=sign*34.f*spread;
        const float x0=x-w*0.5f,x1=x+w*0.5f,y0=y-h*0.5f,y1=y+h*0.5f;
        const ImVec2 a=V(x0,y0-skew*0.45f),b=V(x1,y0+skew*0.45f);
        const ImVec2 c=V(x1,y1-skew*0.45f),d=V(x0,y1+skew*0.45f);
        const bool selected=i==sh.focus && dist<0.52f && !lib.details;
        // Use Flycast's normal imageFill() via drawCover(): its texture
        // coordinates/aspect crop are handled correctly by the PS5 renderer.
        // The previous AddImageQuad drew art at 1/2 size in the top-left.
        softDisc(dl,x,floor+22.f,150.f*scale,col::rgba(12,29,48,60),18);
        if(spread>0.04f)
        {
            const float side=delta<0?-9.f:9.f;
            quad(dl,V(a.x+side,a.y+3),a,d,V(d.x+side,d.y-3),
                col::rgba(61,79,111,242),col::rgba(61,79,111,242),
                col::rgba(25,40,59,245),col::rgba(44,64,91,245));
        }
        if(selected)
            glow(dl,x0-8,y0-11,w+16,h+22,14,
                alpha(col::accent,0.73f),24);
        drawCover(dl,g,x0,y0,w,h,7.f,std::clamp(1.f-dist,0.f,1.f));
        dl->AddLine(a,b,col::rgba(221,233,255,175),2.f*S);
        dl->AddLine(b,c,col::rgba(103,130,166,205),2.f*S);
        dl->AddLine(c,d,col::rgba(87,114,152,194),2.f*S);
        dl->AddLine(d,a,col::rgba(192,217,255,180),2.f*S);
    }
    dl->PopClipRect();
    const std::string label=(selectDisk?"CHOOSE DISC":"DREAMCAST LIBRARY")
        +std::string("   /   ")+sh.name;
    text(dl,bold(),26,96.f,148.f,col::dim,label.c_str());
    const std::string count=std::to_string(n?sh.focus+1:0)+" / "+std::to_string(n);
    const ImVec2 ps=textSize(regular(),23,count.c_str());
    text(dl,regular(),23,W-96-ps.x,148.f,col::dim,count.c_str());
    if(focused)
    {
        const std::string title=fit(bold(),45,focused->title,W-240.f);
        const ImVec2 ts=textSize(bold(),45,title.c_str());
        text(dl,bold(),45,(W-ts.x)*0.5f,851.f,col::text,title.c_str());
        const std::string meta=metaText(*focused);
        const ImVec2 ms=textSize(regular(),23,meta.c_str());
        text(dl,regular(),23,(W-ms.x)*0.5f,910.f,col::dim,meta.c_str());
    }
}
