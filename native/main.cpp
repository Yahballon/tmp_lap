// SPDX-License-Identifier: GPL-3.0-or-later
// Native 1920x1080 PS5 Crypto Tracker, built on blackbearreloaded/ps5-homebrew-ui.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "market_client.hpp"
#include "pin_settings.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using hui::gfx::Color;
using hui::gfx::Rect;
using hui::gfx::DrawList;
static const Color white=Color::rgb(0xf8f9fc), muted=Color::rgb(0xa0b8df),
 panel=Color::rgb(0x10254a), red=Color::rgb(0xff445a), green=Color::rgb(0x22dc91),
 accent=Color::rgb(0x57bfff);
static std::string read(const char* file) {
    FILE* f=fopen(file,"rb");if(!f)return {};
    fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
    if(n<=0||n>16000000){fclose(f);return {};}
    std::string str((size_t)n,'\0');fread(str.data(),1,str.size(),f);fclose(f);return str;
}
static std::string cash(double n) {
    char buf[64];
    if(n>=1000)std::snprintf(buf,sizeof(buf),"$%.2f",n);
    else if(n>=1)std::snprintf(buf,sizeof(buf),"$%.2f",n);
    else std::snprintf(buf,sizeof(buf),"$%.6f",n);
    return buf;
}
static std::string count(double n) {
    char buf[48];
    if(n>=1e12)std::snprintf(buf,sizeof(buf),"%.2fT",n/1e12);
    else if(n>=1e9)std::snprintf(buf,sizeof(buf),"%.2fB",n/1e9);
    else if(n>=1e6)std::snprintf(buf,sizeof(buf),"%.2fM",n/1e6);
    else std::snprintf(buf,sizeof(buf),"%.0f",n);
    return buf;
}
static std::string pct(double n) {
    char buf[64];std::snprintf(buf,sizeof(buf),"%+.2f%%",n);return buf;
}
static void line_chart(DrawList& ui,const std::vector<double>& v,Rect r,Color color,bool fill=false) {
    if(v.size()<2)return;
    auto bounds=std::minmax_element(v.begin(),v.end());
    double lo=*bounds.first,hi=*bounds.second;
    if(hi-lo<1e-10)hi=lo+1;
    const size_t step=std::max<size_t>(1,v.size()/140);
    float priorX=r.x,priorY=r.y+r.h*.5f;
    bool first=true;
    for(size_t i=0;i<v.size();i+=step){
        float x=r.x+r.w*(float)i/(float)(v.size()-1);
        float y=r.y+r.h-(float)((v[i]-lo)/(hi-lo))*r.h;
        if(!first){
            if(fill)ui.line(priorX,r.y+r.h,x,r.y+r.h,1,color.with_alpha(.09f));
            ui.line(priorX,priorY,x,y,2.5f,color);
        }
        first=false;priorX=x;priorY=y;
    }
}
static void text(DrawList& ui,const hui::gfx::Font& font,unsigned atlas,
 std::string_view value,float x,float y,float size,Color c=white,
 hui::gfx::Align a=hui::gfx::Align::left) {ui.text(font,atlas,value,x,y,size,c,a);}
static void coin_icon(DrawList& ui,float x,float y,const tracker::Coin& coin){
    Color bg=Color::rgb(0x3f506b);
    if(coin.symbol=="BTC")bg=Color::rgb(0xf7931a);
    if(coin.symbol=="ETH")bg=Color::rgb(0x6574bb);
    if(coin.symbol=="LTC")bg=Color::rgb(0x858585);
    if(coin.symbol=="USDT")bg=Color::rgb(0x159b85);
    if(coin.symbol=="BNB")bg=Color::rgb(0xf3ba2f);
    if(coin.symbol=="XRP")bg=Color::rgb(0x43485a);
    if(coin.symbol=="SOL")bg=Color::rgb(0x7559bf);
    ui.circle(x,y,29,bg);
}
static bool pinned(const tracker::Coin& c) {
    return c.id=="bitcoin" || c.id=="ethereum" || c.id=="litecoin";
}
static void card(DrawList& ui,const hui::gfx::Font& f,unsigned atlas,
 const tracker::Coin& c,float x,float y,bool selected) {
    ui.shadow({x,y,470,185},22,22,Color::rgb(0x0060b0,0.15f));
    ui.bordered_rect({x,y,470,185},22,panel,selected?3:1,selected?accent:Color::rgb(0x245c9b));
    ui.bordered_rect({x+389,y+16,62,26},7,Color::rgb(0x1c3964),1,Color::rgb(0x3976b8));
    text(ui,f,atlas,"24H",x+420,y+36,15,muted,hui::gfx::Align::center);
    coin_icon(ui,x+54,y+58,c);
    text(ui,f,atlas,c.name,x+100,y+60,26);
    text(ui,f,atlas,c.symbol,x+100,y+91,18,muted);
    text(ui,f,atlas,cash(c.price),x+32,y+137,32);
    text(ui,f,atlas,pct(c.change24h),x+32,y+169,20,c.change24h>=0?green:red);
    line_chart(ui,c.spark,{x+300,y+111,143,60},c.change24h>=0?green:red);
}
int main() {
    hui::ps5::Display display;if(!display.open())hui::sys::park();
    hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
    hui::gfx::Font font;
    std::string fontFile=read("/app0/assets/fonts/inter-semibold.huifont");
    if(fontFile.empty())fontFile=read("/data/homebrew/PPSA99442/assets/fonts/inter-semibold.huifont");
    if(!font.load(fontFile))hui::sys::park();
    unsigned atlas=renderer.batch().create_font_texture(font);
    hui::ps5::Pad pad;pad.open();
    tracker::PinSettings pins;pins.load();
    tracker::requestMarket();
    std::string detailId;
    bool detail=false;int selection=0;int offset=0;
    int daysIndex=0;constexpr int periods[]={1,7,30,90,365};
    unsigned prev=0;int tick=0;std::vector<int> order;
    for(;;) {
        std::array<hui::PadSample,64> samples{};
        unsigned pressed=0;size_t n=pad.read(samples);
        for(size_t i=0;i<n;i++){pressed|=samples[i].buttons&~prev;prev=samples[i].buttons;}
        if(pressed&hui::pad_bits::kCircle) {
            if(detail)detail=false;
            else {pad.close();renderer.release();display.close();hui::sys::quit();}
        }
        auto snap=tracker::snapshot();
        // Present pinned favourites first, then rank-ordered market entries.
        order.clear();
        for(const auto& id:pins.ids)for(size_t i=0;i<snap.coins.size();++i)if(snap.coins[i].id==id)order.push_back((int)i);
        for(size_t i=0;i<snap.coins.size();++i)if(!pins.contains(snap.coins[i].id))order.push_back((int)i);
        const int total=(int)order.size();
        if(!detail) {
            if(pressed&hui::pad_bits::kLeft)selection=std::max(0,selection-1);
            if(pressed&hui::pad_bits::kRight)selection=std::min(total-1,selection+1);
            if(pressed&hui::pad_bits::kUp)selection=std::max(0,selection-3);
            if(pressed&hui::pad_bits::kDown)selection=std::min(total-1,selection+3);
            if(pressed&hui::pad_bits::kCross && total>0) {
                detailId=snap.coins[order[selection]].id;
                detail=true;daysIndex=0;tracker::requestChart(detailId,periods[daysIndex]);
            }
            if(selection<offset)offset=selection;
            if(selection>=offset+9)offset=selection-6;
        } else {
            if((pressed&hui::pad_bits::kLeft) && daysIndex>0) {
                --daysIndex;tracker::requestChart(detailId,periods[daysIndex]);
            }
            if((pressed&hui::pad_bits::kRight) && daysIndex<4) {
                ++daysIndex;tracker::requestChart(detailId,periods[daysIndex]);
            }
        }
        if(pressed&hui::pad_bits::kSquare) {
            std::string id;
            if(detail)id=detailId;
            else if(selection>=0 && selection<total)id=snap.coins[order[selection]].id;
            if(!id.empty() && pins.toggle(id))pins.save();
        }
        if(pressed&hui::pad_bits::kTriangle)tracker::requestMarket();
        if(++tick>=3600){tick=0;if(!snap.busy)tracker::requestMarket();}
        DrawList ui;
        ui.gradient_rect({0,0,1920,1080},0,Color::rgb(0x030a1c),Color::rgb(0x071b43));
        ui.circle(1700,870,720,Color::rgb(0x07183a,0.30f));
        ui.arc(1650,830,650,3,0,6.28f,Color::rgb(0x066aca,0.3f));
        ui.rounded_rect({0,0,1920,52},0,Color::rgb(0x06132b));
        ui.line(0,52,1920,52,2,accent);
        ui.bordered_rect({66,13,29,29},6,Color::rgb(0x07315a),2,accent);
        ui.line(72,34,81,23,3,accent);ui.line(81,23,90,29,3,accent);
        text(ui,font,atlas,"CRYPTO TRACKER",95,36,19,accent);
        ui.bordered_rect({510,12,147,29},8,Color::rgb(0x153047),1,Color::rgb(0x398ac0));
        text(ui,font,atlas,"v0.2.0",583,33,19,Color::rgb(0xffffff),hui::gfx::Align::center);
        text(ui,font,atlas,"TRIANGLE Refresh   X Select   SQUARE Pin / Unpin   CIRCLE Back",1770,36,18,muted,hui::gfx::Align::right);
        if(!detail) {
            text(ui,font,atlas,"Home",190,128,48);
            ui.rounded_rect({180,87,4,47},2,accent);
            text(ui,font,atlas,snap.status,1710,123,21,snap.busy?accent:muted,hui::gfx::Align::right);
            if(total==0) {
                text(ui,font,atlas,"Waiting for live prices from CoinGecko",960,460,32,muted,hui::gfx::Align::center);
                text(ui,font,atlas,"An internet connection and valid TLS certificates are required",960,515,21,muted,hui::gfx::Align::center);
            } else {
                text(ui,font,atlas,"Pinned ("+std::to_string(pins.ids.size())+")",190,199,30,muted);
                for(int i=0;i<total && i<3 && i<(int)pins.ids.size();i++)
                    card(ui,font,atlas,snap.coins[order[i]],190+(i%3)*490,223,i==selection);
                text(ui,font,atlas,"Top 100",190,467,30,muted);
                int visible=0;
                for(int j=std::max((int)pins.ids.size(),offset);j<total && visible<6;j++,visible++) {
                    const int col=visible%3,row=visible/3;
                    card(ui,font,atlas,snap.coins[order[j]],190+col*490,493+row*202,j==selection);
                }
                text(ui,font,atlas,"D-PAD Navigate    X Details    SQUARE Pin / Unpin    TRIANGLE Refresh",190,1041,19,muted);
            }
        } else {
            auto it=std::find_if(snap.coins.begin(),snap.coins.end(),[&](const tracker::Coin& c){return c.id==detailId;});
            if(it==snap.coins.end()) {detail=false;}
            else {
                const tracker::Coin& coin=*it;
                text(ui,font,atlas,"< Back",191,115,23,muted);
                coin_icon(ui,225,186,coin);
                text(ui,font,atlas,coin.name,280,196,46);
                text(ui,font,atlas,coin.symbol,283,229,24,muted);
                text(ui,font,atlas,pins.contains(coin.id)?"PINNED  [SQUARE TO UNPIN]":"SQUARE TO PIN",1060,196,20,pins.contains(coin.id)?accent:muted,hui::gfx::Align::right);
                text(ui,font,atlas,cash(coin.price),190,319,57);
                text(ui,font,atlas,pct(coin.change24h)+"  (24h)",196,363,23,coin.change24h>=0?green:red);
                const float chartX=195,chartY=463,chartW=1000,chartH=420;
                for(int k=0;k<5;k++)ui.line(chartX,chartY+k*(chartH/4),chartX+chartW,chartY+k*(chartH/4),1,Color::rgb(0x222328));
                const bool historic=snap.chartLoaded && snap.chartCoin==coin.id && snap.chartDays==periods[daysIndex];
                const auto& graph=historic?snap.chart:coin.spark;
                line_chart(ui,graph,{chartX,chartY,chartW,chartH},coin.change24h>=0?green:red);
                if(!historic)text(ui,font,atlas,"7D sparkline preview (historical chart not loaded)",201,918,18,muted);
                static const char* labels[]={"1D","7D","30D","90D","1Y"};
                for(int i=0;i<5;i++) {
                    ui.bordered_rect({194.0f+i*105.0f,950,88,49},12,
                       i==daysIndex?Color::rgb(0x383a40):panel,1,
                       i==daysIndex?accent:Color::rgb(0x25252b));
                    text(ui,font,atlas,labels[i],238+i*105,983,21,white,hui::gfx::Align::center);
                }
                ui.bordered_rect({1275,170,450,560},24,panel,2,Color::rgb(0x238eee));
                text(ui,font,atlas,"Market Stats",1307,218,28,muted);
                auto row=[&](std::string label,std::string value,int line){
                    float y=287+(float)line*79;
                    text(ui,font,atlas,label,1309,y,21,muted);
                    text(ui,font,atlas,value,1702,y,22,white,hui::gfx::Align::right);
                    ui.line(1308,y+25,1697,y+25,1,Color::rgb(0x2b2b30));
                };
                row("Market Cap",cash(coin.marketCap),0);
                row("Volume (24h)",cash(coin.volume24h),1);
                row("Circulating",count(coin.circulating),2);
                row("Total Supply",count(coin.totalSupply),3);
                row("All Time High",cash(coin.ath),4);
                row("All Time Low",cash(coin.atl),5);
                text(ui,font,atlas,snap.status,1278,805,20,muted);
                text(ui,font,atlas,"LEFT/RIGHT Chart range     SQUARE Pin / Unpin",193,1040,20,muted);
            }
        }
        renderer.begin();renderer.draw(ui);
        renderer.present(0,display.width(),display.height());display.swap();
        hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
    }
}
