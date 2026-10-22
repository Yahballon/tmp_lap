// Stage pack integration: actual pinned original level authored terrain,
// real collider, artist atlases and native gameplay subsets.
#include "level_preview.hpp"
#include "level_entities.hpp"
#include "level_hazards.hpp"
#include "stage_progress.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
int main(int argc,char**argv){
    if(argc<3)return 2;
    hui::gfx::Renderer renderer;
    const std::string game=argv[1];
    const std::string cache=argv[2];
    const char* stages[]={"demo-1","sunshine-1","sunshine-2","sunshine-3"};
    const int min_bricks[]={1323,1791,2938,634};
    for(int i=0;i<4;++i){
        surgepreview::LevelPreview map;
        surgepreview::LevelEntities items;
        surgepreview::LevelHazards enemies;
        const std::string stage=i==0?"":stages[i];
        if(!map.load(renderer,game,stage)){
            std::fprintf(stderr,"LEVEL %s FAILED: %s\n",stages[i],map.error().c_str());return 3;
        }
        if(int(map.count())!=min_bricks[i]){
            std::fprintf(stderr,"LEVEL %s WRONG BRICKS: %zu\n",stages[i],map.count());return 4;
        }
        if(!items.load(renderer,game,stage)){
            std::fprintf(stderr,"ENTITIES %s FAILED: %s\n",stages[i],items.error().c_str());return 5;
        }
        if(!enemies.load(renderer,game,stage)){
            std::fprintf(stderr,"HAZARDS %s FAILED: %s\n",stages[i],enemies.error().c_str());return 6;
        }
        assert(map.player_y()>9000.f && map.grounded());
        assert(items.count()>=15);
        for(int frame=0;frame<30;++frame)
            map.update(1.f/120.f,1,false);
        assert(std::isfinite(map.player_x()) && std::isfinite(map.player_y()));
        std::printf("STAGE_PASS %s bricks=%zu items=%zu hazards=%zu\n",
                 stages[i],map.count(),items.count(),enemies.count());
    }
    surgepreview::Progress state;
    state.selected=3;
    state.checkpoint(2,500.f,9900.f);
    state.complete_stage(2,700);
    assert(state.selected==3 && state.unlocked==3);
    assert(state.checkpoint_stage==-1 && state.best_score==700);
    const auto filename=cache+"/opensurge-stage-save-smoke.dat";
    assert(state.save(filename));
    surgepreview::Progress loaded;
    assert(loaded.load(filename));
    assert(loaded.selected==3&&loaded.unlocked==3&&loaded.best_score==700);
    loaded.selected=2;loaded.checkpoint(2,300.f,9840.f);
    assert(loaded.save(filename));
    surgepreview::Progress reloaded;
    assert(reloaded.load(filename) && reloaded.checkpoint_stage==2 &&
           reloaded.checkpoint_x==300.f);
    std::remove(filename.c_str());
    std::puts("PASS ORIGINAL MULTI-STAGE PACK + CHECKPOINT/AUTOSAVE");
}
