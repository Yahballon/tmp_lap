// Regression based on a PS5 firmware 5.10 screenshot: at (2500,10015)
// the player is grounded, FPS=60, but VX gets set to zero on the ramp.
// Exercise the actual native mask/physics class and authentic demo data.
#define SURGEPREVIEW_HOST_TEST 1
#include "level_preview.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    hui::gfx::Renderer renderer;
    surgepreview::LevelPreview level;
    if(!level.load(renderer,argv[1])){
        std::fprintf(stderr,"level load: %s\n",level.error().c_str());
        return 3;
    }
    if(!level.test_position(2500.f,10015.f)){
        std::fprintf(stderr,"cannot place grounded player at screenshot x=2500 y~10015\n");
        return 4;
    }
    const float initial=level.player_x();
    std::printf("UPHILL_INIT x=%.1f feet=%.1f grounded=%d\n",
         level.player_x(),level.player_y(),level.grounded()?1:0);
    constexpr float dt=1.f/120.f;
    // DOWN+X initiates Spin Dash from level position shown in screenshot.
    level.update(dt,0,true,true,false);
    if(!level.charging())return 5;
    for(int i=0;i<4;++i){
        level.update(dt,0,true,true,false);
        level.update(dt,0,false,true,false);
    }
    level.update(dt,0,false,false,false);
    const float launch=level.velocity_x();
    std::printf("UPHILL_RELEASE vx=%.1f rolling=%d releases=%u grounded=%d y=%.1f blocks=%u\n",
        launch,level.rolling()?1:0,level.dash_releases(),level.grounded()?1:0,
        level.player_y(),level.collision_blocks());
    for(float off:{0.f,1.f,2.f,4.f,7.f,10.f,16.f,24.f,36.f})level.test_probe(off);
    for(float yy:{9900.f,9930.f,9950.f,9970.f,9980.f,9990.f,10000.f,10012.f})
        level.test_pixel(2517.f,yy);
    if(level.dash_releases()!=1 || !level.rolling())return 6;
    float maxx=level.player_x(), minvx=launch;
    for(int i=0;i<190;++i){
        level.update(dt,1,false,false,false);
        maxx=std::max(maxx,level.player_x());
        minvx=std::min(minvx,level.velocity_x());
        if(i%30==29)
            std::printf("UPHILL_TRACE t=%.2f x=%.1f y=%.1f vx=%.1f slope=%.2f blocks=%u uphill=%u rolling=%d\n",
                (i+1)*dt,level.player_x(),level.player_y(),
                level.velocity_x(),level.slope_grade(),level.collision_blocks(),
                level.uphill_steps(),level.rolling()?1:0);
        if(!std::isfinite(level.player_x())||!std::isfinite(level.player_y()))return 7;
    }
    std::printf("UPHILL_RESULT initial=%.1f maxx=%.1f moved=%.1f launch=%.1f minvx=%.1f blocked=%u uphill_steps=%u\n",
        initial,maxx,maxx-initial,launch,minvx,
        level.collision_blocks(),level.uphill_steps());
    if(maxx-initial<220.f){
        std::fprintf(stderr,"FAIL: Charged Spin Dash stalled halfway up original incline\n");
        return 8;
    }
    if(level.uphill_steps()==0){
        std::fprintf(stderr,"FAIL: Slope was not climbed, collision/motion was bypassed\n");
        return 9;
    }
    // No unsolicited jumps, no controller changes. Reset remains reliable.
    level.reset();
    if(!level.grounded()||level.charging()||level.rolling())return 10;
    std::puts("PASS original uphill ramp: charged Spin Dash gains height and crosses slope");
}
