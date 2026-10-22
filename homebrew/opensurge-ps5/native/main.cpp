// SPDX-License-Identifier: GPL-3.0-or-later
// Open Surge PS5 v0.6.3 - safe original mask physics, wall-climb clipping guard and fall recovery.
// NOT the full original Allegro/SurgeScript gameplay engine.
#include "gfx/renderer.hpp"
#include "level_preview.hpp"
#include "level_entities.hpp"
#include "level_hazards.hpp"
#include "game_audio.hpp"
#include "stage_progress.hpp"
#include "pad_state.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "vendor/stb_image.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/stat.h>

namespace {
using namespace hui::gfx;
constexpr const char* kVersion="0.6.3";
constexpr const char* kSave="/data/OpenSurge/progress.cfg";
constexpr std::array<const char*,4> kStageNames={"demo-1","sunshine-1","sunshine-2","sunshine-3"};
constexpr const char* kTitle="OPEN SURGE PS5 - WORLD ROUTES";
constexpr const char* kData="/data/OpenSurge";
constexpr const char* kLog="/data/OpenSurge/legacy-port.log";
Color C(unsigned hex,float opacity=1.f){return Color::rgb(hex,opacity);}
const Color WHITE=C(0xecf9ff),DIM=C(0xb0bfcf),ACCENT=C(0x66ebac),GOLD=C(0xfbd27e);

void log_message(const std::string& msg){
  mkdir(kData,0755);
  if(FILE* f=fopen(kLog,"ab")){
    fprintf(f,"[OpenSurge multi-stage preview 0.6.3] %s\n",msg.c_str());
    fclose(f);
  }
}
std::string bytes(const std::string& filename,size_t limit){
  FILE* f=fopen(filename.c_str(),"rb");if(!f)return {};
  if(fseek(f,0,SEEK_END)){fclose(f);return {};}
  const long n=ftell(f);
  if(n<1 || static_cast<size_t>(n)>limit){fclose(f);return {};}
  rewind(f);
  std::string data(static_cast<size_t>(n),'\0');
  if(fread(data.data(),1,data.size(),f)!=data.size())data.clear();
  fclose(f);
  return data;
}
std::string asset(const char* filename,size_t limit=8*1024*1024){
  for(const char* p:{"/app0/assets/","/data/homebrew/PPSA99358/assets/",
                     "/mnt/usb0/homebrew/PPSA99358/assets/"}){
    auto value=bytes(std::string(p)+filename,limit);
    if(!value.empty())return value;
  }
  return {};
}
std::string resolve_root(){
  for(const char* root:{"/app0/assets/game/","/data/homebrew/PPSA99358/assets/game/",
                        "/mnt/usb0/homebrew/PPSA99358/assets/game/"}){
    if(FILE* f=fopen((std::string(root)+"surge.cfg").c_str(),"rb")){
      fclose(f);return root;
    }
  }
  return {};
}
struct Probe{std::string kind,path;bool ok=false;size_t first_read=0;};
std::vector<Probe> load_probes(){
  auto list=asset("probe_paths.txt",64*1024);
  std::vector<Probe> results;
  size_t p=0;
  while(p<list.size() && results.size()<12){
    const size_t e=list.find('\n',p),last=e==std::string::npos?list.size():e;
    std::string line=list.substr(p,last-p);
    const auto sep=line.find('\t');
    if(sep!=std::string::npos && sep>0 && sep<line.size()-1){
      std::string kind=line.substr(0,sep),rel=line.substr(sep+1);
      // Only accept relative paths published by our build-time manifest.
      if(rel.find("..")==std::string::npos && rel[0]!='/' && rel.size()<200)
        results.push_back({kind,rel});
    }
    p=last+1;
  }
  return results;
}
void run_probes(std::vector<Probe>& probes,const std::string& root){
  for(auto& row:probes){
    row.ok=false;row.first_read=0;
    if(root.empty())continue;
    const std::string full=root+row.path;
    if(FILE* f=fopen(full.c_str(),"rb")){
      char buffer[512];
      row.first_read=fread(buffer,1,sizeof(buffer),f);
      row.ok=row.first_read>0;
      fclose(f);
    }
    log_message(std::string(row.ok?"ASSET_READ_OK ":"ASSET_READ_FAIL ")+row.path);
  }
}
void txt(DrawList& ui,const Font& font,unsigned atlas,const std::string& text,
         float x,float y,float size,Color color=WHITE,Align align=Align::left){
  ui.text(font,atlas,text,x,y,size,color,align);
}
unsigned image(Renderer& renderer){
  std::string jpg=asset("surge-preview.jpg",12*1024*1024);
  if(jpg.empty()){log_message("PREVIEW_MISSING");return 0;}
  int w=0,h=0,n=0;
  auto* pixel=stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(jpg.data()),
                                   static_cast<int>(jpg.size()),&w,&h,&n,4);
  if(!pixel||w<8||h<8||w>4096||h>4096){
    if(pixel)stbi_image_free(pixel);log_message("PREVIEW_DECODE_FAIL");return 0;
  }
  const unsigned texture=renderer.batch().create_texture(w,h,pixel);
  stbi_image_free(pixel);
  return texture;
}
void draw(DrawList& ui,const Font& font,unsigned atlas,unsigned preview,
          const std::vector<Probe>& probes,const std::string& root,bool detail,bool exiting){
  ui.gradient_rect({0,0,1920,1080},0,C(0x071827),C(0x0b1b33));
  ui.rounded_rect({62,58,1794,164},22,C(0x112b36,.96f));
  txt(ui,font,atlas,"OPEN SURGE",94,142,74,WHITE);
  txt(ui,font,atlas,"by YAHBALLON  /  native PS5 / old DevBridge pipeline",101,192,25,ACCENT);
  txt(ui,font,atlas,"v0.6.3",1807,143,27,DIM,Align::right);
  ui.bordered_rect({64,249,884,687},18,C(0x102631,.96f),2,C(0x236a69));
  ui.bordered_rect({970,249,884,687},18,C(0x102631,.96f),2,C(0x236a69));
  txt(ui,font,atlas,"ORIGINAL GAME DATA",103,317,37,ACCENT);
  txt(ui,font,atlas,root.empty()?"GAME DATA NOT FOUND":"GAME ROOT FOUND",900,312,22,root.empty()?GOLD:ACCENT,Align::right);
  ui.line(105,340,911,340,2,C(0x336f6d));
  if(probes.empty()){
    txt(ui,font,atlas,"Missing probe_paths.txt",110,437,29,GOLD);
  }else{
    for(size_t i=0;i<probes.size() && i<8;++i){
      const float y=369.f+i*67.f;
      const Probe& probe=probes[i];
      ui.rounded_rect({100,y,812,56},9,C(probe.ok?0x154636:0x50311e,.92f));
      txt(ui,font,atlas,probe.ok?"OK":"ERR",126,y+38,23,probe.ok?ACCENT:GOLD);
      const auto label=probe.kind+": "+probe.path;
      txt(ui,font,atlas,label.substr(0,51),205,y+37,20,WHITE);
    }
  }
  if(preview){
    ui.image(preview,{999,275,824,430},{0,0,1,1},WHITE);
  }else{
    txt(ui,font,atlas,"SURGE ART PREVIEW MISSING",1012,474,32,GOLD);
  }
  txt(ui,font,atlas,detail?"LEGACY PS5 GRAPHICS STACK":"RUNTIME TEST",1009,760,28,ACCENT);
  txt(ui,font,atlas,detail?"Renderer: PS5-DevBridge / OpenGL":"Checks native graphics, controller and assets",1009,806,22,WHITE);
  txt(ui,font,atlas,detail?"Controller: DualSense / PS5 pad":"Original gameplay engine: NOT LINKED",1009,847,22,detail?WHITE:GOLD);
  txt(ui,font,atlas,detail?"Audio + level simulation: NOT LINKED":"Actual playable level: NOT IMPLEMENTED",1009,888,22,GOLD);
  txt(ui,font,atlas,"X  LEVEL PREVIEW   |   TRIANGLE  STATUS   |   O  EXIT",960,1005,24,WHITE,Align::center);
  if(exiting){
    ui.rounded_rect({0,0,1920,1080},0,C(0x020a12,.85f));
    ui.bordered_rect({565,340,790,360},18,C(0x102631),3,ACCENT);
    txt(ui,font,atlas,"EXIT OPEN SURGE TEST?",960,476,42,WHITE,Align::center);
    txt(ui,font,atlas,"X Confirm      O Cancel",960,569,28,ACCENT,Align::center);
  }
}
}

int main(){
  log_message("BOOT LEGACY PS5 NATIVE");
  hui::ps5::Display display;
  if(!display.open()){log_message("DISPLAY_OPEN_FAILED");hui::sys::park();}
  Renderer renderer;
  if(!renderer.init()){log_message("RENDERER_INIT_FAILED");hui::sys::park();}
  const auto font_bytes=asset("fonts/inter-semibold.huifont",6*1024*1024);
  Font font;
  if(!font.load(font_bytes)){log_message("FONT_LOAD_FAILED");hui::sys::park();}
  const unsigned atlas=renderer.batch().create_font_texture(font);
  const unsigned preview=image(renderer);
  hui::ps5::Pad pad;pad.open();
  std::string root=resolve_root();
  auto probes=load_probes();run_probes(probes,root);
  surgepreview::GameAudio audio;
  const bool audio_ready=audio.init(root);
  {
    char line[180];
    std::snprintf(line,sizeof(line),"ORIGINAL_AUDIO_INIT %s sfx=%d music=%d output=%d",
                  audio.status().c_str(),audio.clips_loaded(),
                  audio.music_ready()?1:0,audio_ready?1:0);
    log_message(line);
  }
  surgepreview::Progress progress;
  progress.load(kSave);
  int stage_index=progress.selected;
  surgepreview::LevelPreview level;
  bool level_ready=level.load(renderer,root);
  surgepreview::LevelEntities entities;
  bool entities_ready=level_ready && entities.load(renderer,root);
  log_message(entities_ready?
      "ORIGINAL_ENTITIES_READY collectibles, springs and goal":
      std::string("ORIGINAL_ENTITIES_FAILED ")+entities.error());
  surgepreview::LevelHazards hazards;
  bool hazards_ready=level_ready && hazards.load(renderer,root);
  log_message(hazards_ready?
      "ORIGINAL_HAZARDS_READY 5 mosquito 1 wolfey 1 lady_bugsy 6 spikes":
      std::string("ORIGINAL_HAZARDS_FAILED ")+hazards.error());
  log_message(level_ready?
      "LEVEL_PREVIEW_READY original demo-1.lev brick sheet, magenta alpha and collision mask":
      std::string("LEVEL_PREVIEW_FAILED ")+level.error());
  if(level_ready) {
    char line[160];
    std::snprintf(line,sizeof(line),"LEVEL_SPAWN x=%.1f y=%.1f grounded=%d bricks=%zu",
                  level.player_x(),level.player_y(),level.grounded()?1:0,level.count());
    log_message(line);
    std::snprintf(line,sizeof(line),"COLLISION_GRID indexed_tiles=%zu cell_refs=%zu",
                  level.count(),level.indexed_cells());
    log_message(line);
  }
  auto load_stage=[&](int selected)->bool{
    if(selected<0||selected>=int(kStageNames.size()))return false;
    const std::string id=selected==0?"":kStageNames[selected];
    level_ready=level.load(renderer,root,id);
    entities_ready=level_ready && entities.load(renderer,root,id);
    hazards_ready=level_ready && hazards.load(renderer,root,id);
    if(!level_ready){
      log_message(std::string("STAGE_LOAD_FAILED ")+kStageNames[selected]+" "+level.error());
      return false;
    }
    if(progress.checkpoint_stage==selected &&
       !level.set_checkpoint(progress.checkpoint_x,progress.checkpoint_y))
      log_message("CHECKPOINT_UNSAFE; default spawn used");
    char status[160];
    std::snprintf(status,sizeof(status),"STAGE_READY %s bricks=%zu entities=%zu hazards=%zu",
      kStageNames[selected],level.count(),entities_ready?entities.count():0,
      hazards_ready?hazards.count():0);
    log_message(status);
    if(!entities_ready)log_message(std::string("STAGE_ENTITIES_UNAVAILABLE ")+entities.error());
    if(!hazards_ready)log_message(std::string("STAGE_HAZARDS_UNAVAILABLE ")+hazards.error());
    const bool soundtrack=audio.select_stage(selected);
    log_message(std::string("STAGE_MUSIC ")+kStageNames[selected]+" "+
      audio.stage_track()+(soundtrack?" ORIGINAL_TRACK_OK":" ORIGINAL_TRACK_MISSING_FALLBACK"));
    return true;
  };
  bool pending_stage_advance=false;
  bool detail=false,exiting=false,level_mode=false;
  uint64_t last_us=static_cast<uint64_t>(hui::sys::monotonic_us());
  uint64_t fps_sample_us=last_us,perf_log_us=last_us;
  uint64_t last_block_log_us=0;
  unsigned fps_frames=0;
  unsigned last_blocked=0,last_respawns=0;
  unsigned last_layer_switches=0,last_tube_entries=0,last_tube_exits=0;
  unsigned last_unsafe_tubes=0,last_loop_recoveries=0;
  float measured_fps=0.f;
  float accumulator=0.f;
  bool pending_jump=false;
  surgepreview::PadState pad_state;
  for(;;){
    // Record frame start so 16ms isn't added AFTER swap/present anymore.
    const uint64_t frame_start_us=static_cast<uint64_t>(hui::sys::monotonic_us());
    std::array<hui::PadSample,64> samples{};
    const size_t count=pad.read(samples);
    // pad.read()==0 means "no NEW samples", not "nothing is held".
    // This is the actual source of right-walk jitter, repeated jumps and
    // resets even though gameplay still renders at a stable ~60 FPS.
    const auto controls=pad_state.update(samples.data(),count);
    const uint32_t pressed=controls.pressed;
    const uint32_t held=controls.held;
    const uint64_t now=static_cast<uint64_t>(hui::sys::monotonic_us());
    const float dt=std::clamp(float(now-last_us)/1000000.f,0.f,.075f);
    last_us=now;
    ++fps_frames;
    if(now-fps_sample_us>=1000000u){
      measured_fps=float(fps_frames)*1000000.f/float(now-fps_sample_us);
      fps_frames=0;fps_sample_us=now;
    }
    if(level_mode && now-perf_log_us>=5000000u){
      char line[250];
      std::snprintf(line,sizeof(line),
          "PERF fps=%.1f x=%.1f y=%.1f vx=%.1f vy=%.1f grounded=%d cells=%zu blocked=%u respawn=%u slope=%.2f climb=%u wall=%d traversals=%u layer=%d tube=%d deep_restore=%u",
          measured_fps,level.player_x(),level.player_y(),level.velocity_x(),
          level.velocity_y(),level.grounded()?1:0,level.indexed_cells(),
          level.collision_blocks(),level.auto_respawns(),level.slope_grade(),level.uphill_steps(),level.wall_climbing()?1:0,level.wall_climbs(),level.active_layer(),level.in_tube()?1:0,level.solid_recoveries());
      log_message(line);
      std::snprintf(line,sizeof(line),
          "PAD_DIAG raw=0x%08x held=0x%08x empty_polls=%llu samples=%llu disconnects=%llu",
          controls.raw_buttons,controls.held,
          static_cast<unsigned long long>(pad_state.empty_polls()),
          static_cast<unsigned long long>(pad_state.samples_seen()),
          static_cast<unsigned long long>(pad_state.disconnects()));
      log_message(line);
      if(audio_ready){
        std::snprintf(line,sizeof(line),
          "AUDIO_PERF sfx=%d music=%d err=%llu underrun=%llu",
          audio.clips_loaded(),audio.music_enabled()?1:0,
          static_cast<unsigned long long>(audio.output_errors()),
          static_cast<unsigned long long>(audio.underruns()));
        log_message(line);
      }
      perf_log_us=now;
    }
    if(level_mode){
      const auto new_blocked=level.collision_blocks();
      const auto new_respawns=level.auto_respawns();
      if(new_blocked!=last_blocked){
        if((now-last_block_log_us>=700000u) || new_blocked<last_blocked){
          char line[110];
          std::snprintf(line,sizeof(line),"TERRAIN_BLOCK x=%.1f y=%.1f vx=%.1f blocked=%u",
            level.player_x(),level.player_y(),level.velocity_x(),new_blocked);
          log_message(line);
          last_block_log_us=now;
        }
        last_blocked=new_blocked;
      }
      if(new_respawns!=last_respawns){
        log_message("LEVEL_AUTO_RESPAWN after real map gap");
        last_respawns=new_respawns;
      }
    }
    if(level_mode && !exiting){
      if(pressed&hui::pad_bits::kCircle){
        level_mode=false;accumulator=0.f;pending_jump=false;
        audio.level_active(false);
        log_message("LEVEL_PREVIEW_EXIT");
      }else{
        if(pressed&hui::pad_bits::kTriangle){
          const bool on=audio.toggle_music();
          log_message(on?"ORIGINAL_MUSIC_ENABLED":"ORIGINAL_MUSIC_MUTED");
        }
        const int dir=int((held&hui::pad_bits::kRight)!=0)-int((held&hui::pad_bits::kLeft)!=0);
        if(pressed&hui::pad_bits::kCross){
          pending_jump=true;
          char line[112];
          std::snprintf(line,sizeof(line),"PAD_CROSS_EDGE raw=0x%08x held=0x%08x",
              controls.raw_buttons,controls.held);
          log_message(line);
        }
        if(pressed&hui::pad_bits::kSquare){
          char line_btn[112];
          std::snprintf(line_btn,sizeof(line_btn),"PAD_SQUARE_EDGE raw=0x%08x held=0x%08x",
              controls.raw_buttons,controls.held);
          log_message(line_btn);
          level.reset();if(entities_ready)entities.reset();
          if(hazards_ready)hazards.reset_game();
          audio.play(surgepreview::Sound::Checkpoint);
          accumulator=0.f;pending_jump=false;
          char line[160];
          std::snprintf(line,sizeof(line),"LEVEL_RESET x=%.1f y=%.1f grounded=%d",
                        level.player_x(),level.player_y(),level.grounded()?1:0);
          log_message(line);
        }
        // Consistent 120Hz integration independent of GPU/display frame rate.
        // Keeps walking speed stable even if a frame takes 30-40ms.
        constexpr float step=1.f/120.f;
        accumulator=std::min(.075f,accumulator+dt);
        int ticks=0;
        while(accumulator>=step && ticks<9){
          const bool was_grounded=level.grounded();
          const bool jump=pending_jump;
          pending_jump=false;
          const bool was_charging=level.charging();
          const bool was_rolling=level.rolling();
          const unsigned releases_before=level.dash_releases();
          if(!hazards_ready || !hazards.game_over())
            level.update(step,dir,jump,
                         (held&hui::pad_bits::kDown)!=0,
                         (held&hui::pad_bits::kUp)!=0);
          if(!was_charging && level.charging())
          {
             log_message("MOVE_SPINDASH_CHARGE started DOWN+X");
             audio.play(surgepreview::Sound::Charge);
          }
          if(jump && was_charging && level.charging()){
             log_message("MOVE_SPINDASH_RECHARGE X");
             audio.play(surgepreview::Sound::ChargingUp);
          }
          if(level.dash_releases()!=releases_before){
             char dash_log[132];
             std::snprintf(dash_log,sizeof(dash_log),
                           "MOVE_SPINDASH_RELEASE vx=%.1f",level.velocity_x());
             log_message(dash_log);
             audio.play(surgepreview::Sound::Release);
          }
          if(!was_rolling && level.rolling() && !level.charging()){
             log_message("MOVE_ROLL_START");
             audio.play(surgepreview::Sound::Roll);
          }
          if(level.layer_switches()!=last_layer_switches){
             last_layer_switches=level.layer_switches();
             char event[90];
             std::snprintf(event,sizeof(event),"ORIGINAL_LAYER_SWITCH layer=%d changes=%u",
                           level.active_layer(),last_layer_switches);
             log_message(event);
          }
          if(level.tube_entries()!=last_tube_entries){
             last_tube_entries=level.tube_entries();
             log_message("ORIGINAL_TUBE_IN_SCRIPT controls locked");
          }
          if(level.unsafe_tube_exits()!=last_unsafe_tubes){
             last_unsafe_tubes=level.unsafe_tube_exits();
             log_message("TUBE_UNSAFE_EXIT refusing outlet without valid landing");
          }
          if(level.loop_safety_recoveries()!=last_loop_recoveries){
             last_loop_recoveries=level.loop_safety_recoveries();
             log_message("LOOP_GUARD_RESTORE last grounded point; loop physics pending");
          }
          if(level.tube_exits()!=last_tube_exits){
             last_tube_exits=level.tube_exits();
             log_message("ORIGINAL_TUBE_OUT_SCRIPT speed=600");
          }
          if(entities_ready && !level.in_tube()){
            const auto event=entities.tick(step,level.player_x(),level.player_y(),
                                            level.velocity_y());
            if(event.collected){
              char line[120];
              std::snprintf(line,sizeof(line),"COLLECTIBLE_PICKUP gained=%d total=%d",
                            event.collected,entities.collected());
              log_message(line);
              audio.play(surgepreview::Sound::Collectible,0.48f);
            }
            if(event.spring_power>0.f){
              audio.play(surgepreview::Sound::Spring);
              level.spring_bounce(event.spring_power);
              log_message(event.spring_power>800.f?
                 "ORIGINAL_SPRING_STRONGER_BOUNCE 960":
                 "ORIGINAL_SPRING_STANDARD_BOUNCE 600");
            }
            if(event.checkpoint){
              if(level.set_checkpoint(event.checkpoint_x,event.checkpoint_y) &&
                 progress.checkpoint(stage_index,event.checkpoint_x,event.checkpoint_y)){
                log_message(progress.save(kSave)?
                    "ORIGINAL_CHECKPOINT_SAVED":"SAVE_CHECKPOINT_FAILED");
                audio.play(surgepreview::Sound::Checkpoint);
              }else log_message("CHECKPOINT_SKIPPED_UNSAFE_COLLISION");
            }
            if(event.goal){
              log_message(std::string("STAGE_GOAL_REACHED ")+kStageNames[stage_index]);
              audio.play(surgepreview::Sound::Endsign);
              pending_stage_advance=true;
            }
          }
          if(hazards_ready && !hazards.game_over() && !level.in_tube()){
            const bool attacking=level.rolling()||
                (!level.grounded() && level.velocity_y()>40.f);
            const int rings=entities_ready?entities.collected():0;
            const auto damage=hazards.tick(step,level.player_x(),level.player_y(),
                                            level.velocity_y(),attacking,rings);
            if(damage.killed){
              char line[100];
              std::snprintf(line,sizeof(line),"ENEMY_DEFEATED total=%d score=%d",
                           hazards.defeated(),hazards.score());
              log_message(line);
              audio.play(surgepreview::Sound::Destroy);
            }
            if(damage.hit){
              if(entities_ready && rings>0){
                const int lost=entities.lose_rings();
                char line[100];
                std::snprintf(line,sizeof(line),"PLAYER_HIT_RING_LOSS count=%d",lost);
                log_message(line);
                audio.play(surgepreview::Sound::CollectibleLoss);
                level.hit_recoil(damage.source_x);
              }else{
                char line[100];
                std::snprintf(line,sizeof(line),"PLAYER_LOST_LIFE remaining=%d",hazards.lives());
                log_message(line);
                audio.play(surgepreview::Sound::Death);
                if(!hazards.game_over()){
                  level.reset();
                  if(entities_ready)entities.reset();
                  hazards.restart_level();
                }else log_message("PLAYER_GAME_OVER press SQUARE to retry");
              }
            }
          }
          if(jump && was_grounded && !level.charging()){
            log_message("LEVEL_JUMP");
            if(level.velocity_y()<0.f)audio.play(surgepreview::Sound::Jump);
          }
          if(!was_grounded&&level.grounded())log_message("LEVEL_LANDED");
          accumulator-=step;++ticks;
        }
        if(ticks>=9)accumulator=0.f; // avoid spiral of death on a stalled GPU
        if(pending_stage_advance){
          pending_stage_advance=false;
          const int completed=stage_index;
          progress.complete_stage(stage_index,hazards_ready?hazards.score():0);
          if(!progress.save(kSave))log_message("SAVE_STAGE_PROGRESS_FAILED");
          if(completed+1<int(kStageNames.size())){
            stage_index=completed+1;
            if(!load_stage(stage_index)){
              level_mode=false;audio.level_active(false);
            }
            accumulator=0.f;pending_jump=false;
            log_message(std::string("AUTO_NEXT_STAGE ")+kStageNames[stage_index]);
          }else{
            log_message("SUNSHINE_STAGE_BUNDLE_COMPLETE");
            level_mode=false;audio.level_active(false);
          }
        }
      }
    }else if(exiting){
      if(pressed&hui::pad_bits::kCircle)exiting=false;
      if(pressed&hui::pad_bits::kCross){
        log_message("EXIT NATIVE");
        audio.shutdown();
        pad.close();
        renderer.release();display.close();
        hui::sys::quit();
      }
    }else{
      // All four stages unlocked for QA testing; story completion is saved
      // separately so testers can reproduce late-game problems immediately.
      const bool prev=(pressed&(hui::pad_bits::kL1|hui::pad_bits::kLeft))!=0;
      const bool next=(pressed&(hui::pad_bits::kR1|hui::pad_bits::kRight))!=0;
      if(prev!=next){
        stage_index=(stage_index+(next?1:3))%4;
        progress.selected=stage_index;
        if(!progress.save(kSave))log_message("SAVE_STAGE_SELECTION_FAILED");
        log_message(std::string("STAGE_SELECTED ")+kStageNames[stage_index]);
      }
      if(pressed&hui::pad_bits::kCross){
        if(load_stage(stage_index)){
          level_mode=true;level.reset();
          audio.level_active(true);
          accumulator=0.f;pending_jump=false;
          log_message(std::string("STAGE_START ")+kStageNames[stage_index]);
        }else{
          root=resolve_root();run_probes(probes,root);
          log_message(std::string("STAGE_UNAVAILABLE ")+kStageNames[stage_index]);
        }
      }
      if(pressed&hui::pad_bits::kTriangle)detail=!detail;
      if(pressed&hui::pad_bits::kOptions)detail=!detail;
      if(pressed&hui::pad_bits::kCircle)exiting=true;
    }
    audio.pump();
    DrawList ui;
    if(level_mode && level_ready){
      level.draw(ui);
      if(entities_ready)entities.draw(ui,level.player_x(),level.player_y());
      if(hazards_ready)hazards.draw(ui,level.player_x(),level.player_y());
      ui.rounded_rect({36,28,1090,122},14,C(0x072128,.90f));
      txt(ui,font,atlas,std::string("OPEN SURGE  /  ")+kStageNames[stage_index],66,81,33,WHITE);
      txt(ui,font,atlas,"v0.6.3  -  PHYSICS SAFETY / FOUR-STAGE COLLISION QA",66,125,21,GOLD);
      ui.rounded_rect({36,979,1830,73},12,C(0x072128,.94f));
      txt(ui,font,atlas,"MOVE  X JUMP  DOWN+X DASH  TRIANGLE MUSIC  SQUARE RESET  O BACK",73,1029,28,WHITE);
      char probe_line[180];
      std::snprintf(probe_line,sizeof(probe_line),
          "SURGE (%.0f,%.0f) GROUND:%s VX:%+.0f FPS:%.0f",
          level.player_x(),level.player_y(),level.grounded()?"YES":"NO",
          level.velocity_x(),measured_fps);
      ui.rounded_rect({1230,30,630,67},10,C(0x092431,.87f));
      txt(ui,font,atlas,probe_line,1240,72,19,ACCENT);
      if(level.charging()){
        char charge_line[96];
        std::snprintf(charge_line,sizeof(charge_line),
          "SPIN DASH CHARGE  %d%%  |  RELEASE DOWN",
          int(level.dash_charge()*100.f));
        ui.rounded_rect({1208,198,656,64},12,C(0x103f3b,.92f));
        txt(ui,font,atlas,charge_line,1225,240,21,GOLD);
      }else if(level.rolling()){
        ui.rounded_rect({1490,198,374,58},12,C(0x103f3b,.92f));
        txt(ui,font,atlas,"ROLLING",1512,237,23,ACCENT);
      }
      if(entities_ready){
        char hud[140];
        std::snprintf(hud,sizeof(hud),"COLLECTIBLES %03d     %s",
                      entities.collected(),entities.complete()?"GOAL REACHED":"ORIGINAL LEVEL");
        ui.rounded_rect({1240,116,620,62},12,C(0x092431,.90f));
        txt(ui,font,atlas,hud,1258,157,24,entities.complete()?GOLD:ACCENT);
      }
      if(audio_ready){
        const char* sound=audio.music_enabled()&&audio.music_ready()?"MUSIC ON":"MUSIC OFF";
        txt(ui,font,atlas,sound,1812,375,21,ACCENT,Align::right);
      }
      if(hazards_ready){
        char stat[120];
        std::snprintf(stat,sizeof(stat),"LIVES %d     SCORE %d     ENEMIES %d",
                      hazards.lives(),hazards.score(),hazards.defeated());
        ui.rounded_rect({1132,273,726,65},12,C(0x092431,.90f));
        txt(ui,font,atlas,stat,1150,316,23,GOLD);
        if(hazards.game_over()){
          ui.rounded_rect({500,368,920,275},22,C(0x071827,.97f));
          txt(ui,font,atlas,"GAME OVER",960,478,69,GOLD,Align::center);
          txt(ui,font,atlas,"SQUARE  RESTART     CIRCLE  MENU",960,558,26,WHITE,Align::center);
        }
      }
    }else{
      draw(ui,font,atlas,preview,probes,root,detail,exiting);
      ui.rounded_rect({965,875,885,135},14,C(0x0a3139,.97f));
      txt(ui,font,atlas,std::string("STAGE: ")+kStageNames[stage_index]+
          "    L1 / R1  TO CHANGE",985,927,26,ACCENT);
      txt(ui,font,atlas,"X START   |   GOAL NEXT STAGE   |   CHECKPOINT AUTOSAVE",
          985,969,18,GOLD);
      if(!level_ready)
        txt(ui,font,atlas,std::string("LEVEL PREVIEW: ")+level.error(),103,905,19,GOLD);
    }
    renderer.begin();renderer.draw(ui);
    renderer.present(0,display.width(),display.height());
    display.swap();
    hui::sys::hide_splash_screen();
    // Avoid double-throttling: the old unconditional 16ms pause happened
    // AFTER the GPU present/swap and made movement sluggish (~30 FPS).
    const uint64_t frame_elapsed_us=static_cast<uint64_t>(hui::sys::monotonic_us())-frame_start_us;
    if(frame_elapsed_us<16667u)
      hui::sys::sleep_us(16667u-frame_elapsed_us);
  }
}
