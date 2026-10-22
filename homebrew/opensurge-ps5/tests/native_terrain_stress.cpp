// PS5 native player collision smoke/stress across four authentic Sunshine maps.
// Exercise the ORIGINAL pinned Open Surge source collision mask, not mocks of
// the physics implementation. Pure C++ host test, does not ship in FSELF.
#define SURGEPREVIEW_HOST_TEST 1
#include "level_preview.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

int main(int argc,char** argv){
    if(argc!=2)return 2;
    hui::gfx::Renderer renderer;
    const std::string game=argv[1];
    const char* stages[]={"demo-1","sunshine-1","sunshine-2","sunshine-3"};
    constexpr float dt=1.f/120.f;
    for(int stage=0;stage<4;++stage){
        surgepreview::LevelPreview map;
        if(!map.load(renderer,game,stage==0?"":stages[stage])){
            std::fprintf(stderr,"FAILED STRESS LOAD stage=%s reason=%s\n",stages[stage],map.error().c_str());
            return 3;
        }
        map.test_enable_routes(false);
        unsigned wall_ticks=0,max_wall_ticks=0;
        unsigned penetrations=0,consecutive=0,max_consecutive=0;
        for(int frame=0;frame<1140;++frame){
            const int phase=frame%380;
            // Walking, charge/release, idle, jump and reversing to cross
            // platforms and steeper authored ramps in each original stage.
            const int direction=phase<290?1:-1;
            const bool down=phase>=75&&phase<92;
            const bool jump=phase==75||phase==77||phase==79||
                            phase==81||phase==83||phase==85||
                            phase==210||phase==310;
            map.update(dt,direction,jump,down);
            if(!std::isfinite(map.player_x())||!std::isfinite(map.player_y())||
               std::abs(map.player_x())>100000.f||std::abs(map.player_y())>100000.f){
                std::fprintf(stderr,"FAILED NONFINITE stage=%s frame=%d\n",stages[stage],frame);
                return 4;
            }
            if(map.wall_climbing()){
                wall_ticks++;
                max_wall_ticks=wall_ticks>max_wall_ticks?wall_ticks:max_wall_ticks;
            }else wall_ticks=0;
            if(wall_ticks>110){
                std::fprintf(stderr,"FAILED STUCK WALL stage=%s frame=%d x=%.1f y=%.1f\n",
                             stages[stage],frame,map.player_x(),map.player_y());
                return 5;
            }
            if(map.test_inside() && !map.wall_climbing()){
                ++penetrations;++consecutive;
                max_consecutive=std::max(max_consecutive,consecutive);
                if(penetrations<=10)
                    std::printf("TERRAIN_OVERLAP stage=%s frame=%d x=%.1f y=%.1f rolling=%d\n",
                        stages[stage],frame,map.player_x(),map.player_y(),map.rolling()?1:0);
            }else consecutive=0;
        }
        std::printf("TERRAIN_STRESS_PASS stage=%s x=%.1f y=%.1f grounded=%d max_wall_ticks=%u penetration_samples=%u\n",
                    stages[stage],map.player_x(),map.player_y(),map.grounded()?1:0,
                    max_wall_ticks,penetrations);
        if(max_consecutive>12){
            std::fprintf(stderr,"FAILED PERSISTENT SOLID OVERLAP stage=%s ticks=%u samples=%u\n",
                         stages[stage],max_consecutive,penetrations);
            return 6;
        }
        // Repeated pit recovery must be reflected in the diagnostics.
        // Prior reset() zeroed the increment, making respawn logs useless.
        const unsigned before=map.auto_respawns();
        map.test_put(50000.f,40000.f);
        map.update(dt,0,false);
        if(map.auto_respawns()!=before+1 || !map.grounded()){
            std::fprintf(stderr,"FAILED FALL RECOVERY stage=%s count=%u grounded=%d\n",
                         stages[stage],map.auto_respawns(),map.grounded()?1:0);
            return 7;
        }
        map.test_put(50000.f,40000.f);
        map.update(dt,0,false);
        if(map.auto_respawns()!=before+2){
            std::fprintf(stderr,"FAILED RESPAWN COUNTER stage=%s count=%u\n",
                         stages[stage],map.auto_respawns());
            return 8;
        }
    }
    {
        // From the PS5 screenshot/source mask: both bricks 26 and 30 are
        // opaque inside x2517, y9990..10012. An embedded player must
        // recover to safe ground rather than getting permanently trapped.
        surgepreview::LevelPreview embedded;
        if(!embedded.load(renderer,game))return 12;
        embedded.test_enable_routes(false);
        embedded.test_put(2517.f,10015.f);
        for(int tick=0;tick<30;++tick)embedded.update(dt,0,false);
        if(embedded.solid_recoveries()!=1 || embedded.test_inside()){
            std::fprintf(stderr,"FAILED DEEP SOLID RECOVERY count=%u x=%.1f y=%.1f\n",
                         embedded.solid_recoveries(),embedded.player_x(),embedded.player_y());
            return 13;
        }
        std::printf("DEEP_SOLID_RESTORE_PASS count=%u\n",embedded.solid_recoveries());
    }
    std::puts("PASS original four-stage physics stress, solid ascent guard and repeated fall recovery");
    return 0;
}
