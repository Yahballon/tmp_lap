/*
 * PS5DreamcastX - CoverFlow jewel-case presentation.
 * Copyright (C) 2026 YAHBALLON contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Original PS5 UI drawing inspired by console cover-flow libraries:
 * white cases, perspective neighbours, centered hero, short reflection.
 * No third-party logos, game artwork or platform trademarks embedded.
 */

// A trapezoid for a case tilted toward the viewer. The surface is bounded:
// unlike the original stretched shadows, these corners never leave the case.
struct DreamcastXFace { ImVec2 tl,tr,br,bl; };
struct DreamcastXUV { ImVec2 tl,br; };

static DreamcastXFace dreamcastXFace(float x,float y,float w,float h,float angle)
{
    const float a=std::clamp(angle,-1.f,1.f);
    const float far=std::fabs(a)*h*0.075f;
    const float narrow=std::fabs(a)*w*0.23f;
    if (a>0.f) // right shelf, near (left) edge is full height
        return {V(x,y),V(x+w-narrow,y+far),
                V(x+w-narrow,y+h-far),V(x,y+h)};
    if (a<0.f) // left shelf, near (right) edge is full height
        return {V(x+narrow,y+far),V(x+w,y),
                V(x+w,y+h),V(x+narrow,y+h-far)};
    return {V(x,y),V(x+w,y),V(x+w,y+h),V(x,y+h)};
}
static ImVec2 dreamcastXPoint(const DreamcastXFace& f,float u,float v)
{
    // Interpolate within the four corners so case/art/header stay aligned.
    const float ax=f.tl.x+(f.tr.x-f.tl.x)*u;
    const float ay=f.tl.y+(f.tr.y-f.tl.y)*u;
    const float bx=f.bl.x+(f.br.x-f.bl.x)*u;
    const float by=f.bl.y+(f.br.y-f.bl.y)*u;
    return V(ax+(bx-ax)*v,ay+(by-ay)*v);
}
static DreamcastXFace dreamcastXRegion(const DreamcastXFace& f,
                                       float u0,float v0,float u1,float v1)
{
    return {dreamcastXPoint(f,u0,v0),dreamcastXPoint(f,u1,v0),
            dreamcastXPoint(f,u1,v1),dreamcastXPoint(f,u0,v1)};
}
static void dreamcastXPaint(ImDrawList* dl,const DreamcastXFace& f,ImU32 color)
{
    dl->AddQuadFilled(f.tl,f.tr,f.br,f.bl,color);
}
static DreamcastXUV dreamcastXCoverUV(float textureAspect,float windowAspect)
{
    if (!std::isfinite(textureAspect)||textureAspect<=0.f)
        textureAspect=1.f;
    if (!std::isfinite(windowAspect)||windowAspect<=0.f)
        windowAspect=0.72f;
    textureAspect=std::clamp(textureAspect,0.25f,4.f);
    if (textureAspect>windowAspect)
    {
        const float cut=0.5f*(1.f-windowAspect/textureAspect);
        return {ImVec2(cut,0.f),ImVec2(1.f-cut,1.f)};
    }
    const float cut=0.5f*(1.f-textureAspect/windowAspect);
    return {ImVec2(0.f,cut),ImVec2(1.f,1.f-cut)};
}
static void dreamcastXDrawImage(ImDrawList* dl,ImTextureID image,
                                const DreamcastXFace& q,const DreamcastXUV& uv,
                                ImU32 tint)
{
    dl->AddImageQuad(image,q.tl,q.tr,q.br,q.bl,
                     ImVec2(uv.tl.x,uv.tl.y),ImVec2(uv.br.x,uv.tl.y),
                     ImVec2(uv.br.x,uv.br.y),ImVec2(uv.tl.x,uv.br.y),tint);
}
static void dreamcastXReflection(ImDrawList* dl,ImTextureID image,
                                 const DreamcastXFace& face,
                                 const DreamcastXUV& uv,float opacity,float height)
{
    if (image==ImTextureID()||opacity<=0.f) return;
    // Reflection is at most 80 logical pixels below the case; no broad quads.
    const float rh=std::min(80.f,height*0.20f);
    const float topY=std::max(face.bl.y,face.br.y)+6.f;
    const DreamcastXFace r={
        V(face.bl.x,topY),V(face.br.x,topY),
        V(face.br.x,topY+rh),V(face.bl.x,topY+rh)
    };
    // Mirror by reversing the V coordinates. Use the same cropped UV.
    dl->AddImageQuad(image,r.tl,r.tr,r.br,r.bl,
                     ImVec2(uv.tl.x,uv.br.y),ImVec2(uv.br.x,uv.br.y),
                     ImVec2(uv.br.x,uv.tl.y),ImVec2(uv.tl.x,uv.tl.y),
                     col::rgba(255,255,255,(int)(49.f*opacity)));
    // Fade entirely to the floor colour within the bounded reflection strip.
    dl->AddRectFilledMultiColor(V(std::min(r.tl.x,r.tr.x),topY),
                               V(std::max(r.tl.x,r.tr.x),topY+rh),
                               col::rgba(9,16,27,80),col::rgba(9,16,27,80),
                               col::rgba(9,16,27,245),col::rgba(9,16,27,245));
}
static void drawDreamcastXCase(ImDrawList* dl,Game& game,
                               float cx,float cy,float w,float h,
                               float angle,float opacity,bool focused)
{
    const DreamcastXFace face=dreamcastXFace(cx-w*0.5f,cy-h*0.5f,w,h,angle);
    const float fade=std::clamp(opacity,0.f,1.f);
    // Offset at most 5 pixels: this shadow cannot stretch on animation.
    const DreamcastXFace shadow={
        V(face.tl.x+4.f,face.tl.y+5.f),V(face.tr.x+4.f,face.tr.y+5.f),
        V(face.br.x+4.f,face.br.y+5.f),V(face.bl.x+4.f,face.bl.y+5.f)
    };
    dreamcastXPaint(dl,shadow,col::rgba(0,0,0,(int)(58.f*fade)));
    dreamcastXPaint(dl,face,col::rgba(237,241,245,(int)(255.f*fade)));
    // A narrow bevelled plastic spine and the white horizontal header.
    dreamcastXPaint(dl,dreamcastXRegion(face,0.f,0.f,0.055f,1.f),
                    col::rgba(168,179,191,(int)(255.f*fade)));
    dreamcastXPaint(dl,dreamcastXRegion(face,0.052f,0.f,0.065f,1.f),
                    col::rgba(255,255,255,(int)(235.f*fade)));
    const float header=0.075f;
    dreamcastXPaint(dl,dreamcastXRegion(face,0.065f,0.008f,0.988f,header),
                    col::rgba(249,250,252,(int)(255.f*fade)));
    const ImVec2 headerText=dreamcastXPoint(face,0.12f,0.020f);
    text(dl,bold(),std::max(12.f,h*0.035f),headerText.x,headerText.y,
         col::rgba(111,125,143,(int)(255.f*fade)),"Dreamcast");

    // Art runs immediately below the header and occupies almost the whole case.
    const float left=0.073f,right=0.985f,top=header+0.008f,bottom=0.991f;
    const DreamcastXFace imageFace=dreamcastXRegion(face,left,top,right,bottom);
    dreamcastXPaint(dl,imageFace,col::rgba(22,31,44,(int)(255.f*fade)));
    const ImTextureID art=coverTexture(game);
    if (art!=ImTextureID())
    {
        float ratio=imguiDriver!=nullptr?imguiDriver->getAspectRatio(art):1.f;
        // Projected front aspect is the physical DVD-case ratio. Fill, don't
        // stretch square Dreamcast art: crop a little from left/right instead.
        const DreamcastXUV uv=dreamcastXCoverUV(ratio,w*(right-left)/(h*(bottom-top)));
        if (std::fabs(angle)<0.006f)
        {
            // This is the established, PS5-tested texture path for the hero.
            imageFill(dl,art,imageFace.tl.x,imageFace.tl.y,
                      imageFace.tr.x-imageFace.tl.x,imageFace.bl.y-imageFace.tl.y,1.f);
        }
        else
            dreamcastXDrawImage(dl,art,imageFace,uv,
                               col::rgba(255,255,255,(int)(255.f*fade)));
        dreamcastXReflection(dl,art,imageFace,uv,fade*(focused?1.f:0.65f),h);
    }
    else if (std::fabs(angle)<0.006f)
        placeholder(dl,game,imageFace.tl.x,imageFace.tl.y,
                    imageFace.tr.x-imageFace.tl.x,
                    imageFace.bl.y-imageFace.tl.y,2.f);

    // A small gloss strip along the spine, physically inside the case.
    dl->AddLine(dreamcastXPoint(face,0.062f,0.01f),
                dreamcastXPoint(face,0.062f,0.985f),
                col::rgba(255,255,255,(int)(115.f*fade)),1.2f*S);
    dl->AddQuad(face.tl,face.tr,face.br,face.bl,
                focused?alpha(col::accent,0.94f):
                        col::rgba(192,206,218,(int)(205.f*fade)),
                focused?3.5f*S:1.4f*S);
}

void drawDreamcastXShelf(ImDrawList *dl,Game *focused,bool selectDisk)
{
    if (lib.shelves.empty()) return;
    Shelf& shelf=lib.shelves[std::clamp(lib.shelf,0,(int)lib.shelves.size()-1)];
    shelf.focusAnim=motion()==MotionOff||shelf.focusAnim<0.f?
                    (float)shelf.focus:approach(shelf.focusAnim,(float)shelf.focus,13.f);
    const int n=(int)shelf.items.size();
    const float centerX=W*0.5f,centerY=484.f;

    // A shallow stage / reflective floor, not a fake oversized projection.
    gradientV(dl,0.f,705.f,W,105.f,
              col::rgba(11,18,29,0),col::rgba(11,18,29,110));
    const int start=std::max(0,(int)std::floor(shelf.focusAnim)-5);
    const int end=std::min(n-1,(int)std::ceil(shelf.focusAnim)+5);
    std::vector<int> order;
    for(int i=start;i<=end;i++) order.push_back(i);
    std::sort(order.begin(),order.end(),[&](int a,int b){
        return std::fabs(a-shelf.focusAnim)>std::fabs(b-shelf.focusAnim);
    });
    dl->PushClipRect(V(32.f,223.f),V(W-32.f,811.f),true);
    for(int i:order)
    {
        Game& game=games[shelf.items[(size_t)i]];
        const float delta=(float)i-shelf.focusAnim;
        const float dist=std::fabs(delta);
        const float scale=std::max(0.57f,1.f-0.16f*std::min(dist,2.5f));
        const float w=314.f*scale,h=438.f*scale;
        const float x=centerX+delta*268.f;
        const float y=centerY+std::min(dist,3.f)*13.f;
        const float angle=std::clamp(delta*0.68f,-0.83f,0.83f);
        const bool selected=i==shelf.focus&&dist<0.46f&&!lib.details;
        const float fade=std::max(0.56f,1.f-0.11f*dist);
        drawDreamcastXCase(dl,game,x,y,w,h,angle,fade,selected);
    }
    dl->PopClipRect();

    const std::string label=(selectDisk?"CHOOSE DISC":"DREAMCAST LIBRARY")
        +std::string("   /   ")+shelf.name;
    text(dl,bold(),26,96.f,148.f,col::dim,label.c_str());
    const std::string count=std::to_string(n?shelf.focus+1:0)+" / "+std::to_string(n);
    const ImVec2 numberSize=textSize(regular(),23,count.c_str());
    text(dl,regular(),23,W-96.f-numberSize.x,148.f,col::dim,count.c_str());
    if (focused)
    {
        const std::string title=fit(bold(),43,focused->title,W-310.f);
        const ImVec2 size=textSize(bold(),43,title.c_str());
        text(dl,bold(),43,(W-size.x)*0.5f,190.f,col::text,title.c_str());
        const std::string meta=metaText(*focused);
        const ImVec2 metaSize=textSize(regular(),23,meta.c_str());
        text(dl,regular(),23,(W-metaSize.x)*0.5f,865.f,col::dim,meta.c_str());
    }
}
