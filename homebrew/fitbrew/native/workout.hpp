// SPDX-License-Identifier: MIT
// FITBREW v0.1.0. All exercises are bodyweight-only; no equipment required.
#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
namespace fitbrew {
struct Exercise {
 const char* name; const char* area; const char* cue1; const char* cue2;
 const char* easier; const char* warning; uint8_t image; uint8_t impact;
};
inline constexpr Exercise exercises[] = {
 {"March in Place","Warm-up","Stand tall and march gently.","Swing your arms naturally.","Slow marching","Keep a comfortable pace.",3,0},
 {"Arm Circles","Warm-up","Stretch arms to both sides.","Make small, controlled circles.","Smaller circles","Keep shoulders relaxed.",3,0},
 {"Bodyweight Squat","Legs","Feet shoulder-width; hips back.","Keep knees aligned with toes.","Shallow squat","Keep heels grounded.",1,1},
 {"Wall-Free Push-up","Chest","Place hands under shoulders.","Lower chest with a straight trunk.","Knee push-up","Avoid sagging your lower back.",0,1},
 {"Knee Push-up","Chest","Keep hands below shoulders.","Bend elbows slowly; press away.","Short-range knee push-up","Use a comfortable range.",0,0},
 {"Reverse Lunge","Legs","Step backward; drop gently.","Push through your front foot to stand.","Supported static step-back","Reduce depth if knees hurt.",5,1},
 {"Alternating Step-back","Cardio","Step back one foot at a time.","Bring arms overhead if comfortable.","Smaller step","Move slowly without jumping.",3,0},
 {"Forearm Plank","Core","Elbows directly under shoulders.","Brace abs; keep hips level.","Knee plank","Stop if the lower back arches.",2,0},
 {"Knee Plank","Core","Rest forearms and knees down.","Keep shoulders and hips aligned.","Shorter hold","Avoid locking your breath.",2,0},
 {"Glute Bridge","Glutes","Lie on your back, bend knees.","Raise hips using your glutes.","Smaller lift","Do not overarch your back.",2,0},
 {"Dead Bug","Core","Lie on your back, knees over hips.","Extend opposite arm and leg slowly.","Move arms only","Keep your back neutral.",2,0},
 {"Bird Dog","Core","Start on all fours.","Extend opposite arm and leg.","Legs only","Do not twist your hips.",2,0},
 {"Standing Calf Raise","Legs","Stand upright and lift heels.","Lower slowly with control.","Smaller lift","Stay balanced.",5,0},
 {"Side Lunge","Legs","Step sideways; bend one knee.","Keep the other leg comfortably straight.","Smaller side step","Maintain knee alignment.",5,1},
 {"Jumping Jack","Cardio","Jump feet apart while raising arms.","Land softly and repeat.","Step Jack","Avoid high impact if needed.",3,2},
 {"Step Jack","Cardio","Step one foot out to the side.","Raise arms and return; alternate.","Arms only","No jumping required.",3,0},
 {"High Knees","Cardio","Jog in place lifting knees.","Pump arms without leaning back.","March in Place","Keep landings soft.",3,2},
 {"Mountain Climber","Cardio","Start in a straight-arm plank.","Bring one knee forward at a time.","Slow mountain climber","Support shoulders, limit speed.",4,1},
 {"Slow Mountain Climber","Cardio","Start in plank; step knee ahead.","Alternate without bouncing.","Knee plank","Do not force the hip range.",4,0},
 {"Standing Side Crunch","Core","Raise knee toward same-side elbow.","Return upright and alternate.","Small range","Avoid pulling your neck.",3,0},
 {"Superman Lift","Back","Lie face down, arms extended.","Lift arms and legs slightly.","Arms only","Avoid neck hyperextension.",2,0},
 {"Good Morning","Mobility","Stand tall, hands at hips.","Hinge at hips with a flat back.","Shallow hinge","Stop if back pain occurs.",5,0},
 {"Standing Quad Stretch","Cool-down","Stand tall; gently bend one knee.","Hold lightly; switch sides.","Gentle heel lift","Use a stable stance.",5,0},
 {"Child's Pose","Cool-down","Kneel; sit back comfortably.","Reach arms forward and breathe.","Hands under shoulders","Avoid painful knee ranges.",2,0},
 {"Seated Forward Reach","Cool-down","Sit with legs comfortably apart.","Hinge gently, no bouncing.","Bent knees","Never force a stretch.",2,0},
 {"Torso Rotation","Mobility","Stand and rotate slowly.","Keep hips mostly facing front.","Smaller rotation","Avoid fast twisting.",3,0},
 {"Bear Hold","Core","Hands under shoulders, knees bent.","Lift knees slightly; breathe.","Hands-and-knees hold","Keep spine neutral.",4,1},
 {"Reverse Snow Angel","Back","Lie on belly with arms at sides.","Sweep arms slowly toward head.","Smaller arm sweep","Keep neck relaxed.",2,0},
 {"Single-leg Balance","Balance","Stand tall on one leg.","Switch legs halfway through.","Toe on floor","Stay near a clear wall if needed.",5,0},
 {"Standing Hip Opener","Mobility","Lift one knee comfortably.","Rotate out slowly and switch.","Small hip circles","Avoid twisting the planted knee.",5,0},
 {"Skater Step","Cardio","Step diagonally behind one leg.","Reach across the body; alternate.","Small side steps","Stay low-impact and balanced.",3,0},
 {"Plank Shoulder Tap","Core","Start in straight-arm plank.","Tap opposite shoulder slowly.","Knee shoulder tap","Limit shifting of the hips.",4,1},
 {"Side Plank Knees","Core","Lie on side, knees slightly bent.","Lift hips, keep shoulders stacked.","Shorter hold","Switch sides halfway.",2,0},
 {"Frog Squat","Legs","Feet comfortably wider than hips.","Sit hips down then rise.","Standard squat","Do not force knee or hip range.",1,1},
 {"Standing Knee Drive","Cardio","Step back then bring knee upward.","Alternate steadily.","Slow knee lift","Avoid arching lower back.",3,0},
 {"Seated Spine Twist","Cool-down","Sit upright, lengthen spine.","Rotate gently from mid-back.","Small rotation","Never force rotation.",2,0}
};
inline constexpr int EXERCISE_COUNT=sizeof(exercises)/sizeof(exercises[0]);
struct Plan {
 const char* title; const char* subtitle; const char* category; const char* level;
 std::array<uint8_t,10> sequence; uint8_t count; uint8_t seconds; uint8_t rest; uint8_t image;
};
inline constexpr Plan plans[] = {
 {"FULL BODY STARTER","A gentle start for everybody","FULL BODY","BEGINNER",{{0,1,2,4,9,11,12,23,0,0}},8,30,15,0},
 {"LOW IMPACT CARDIO","Raise your pulse without jumps","CARDIO","BEGINNER",{{0,15,6,19,30,34,1,24,0,0}},8,35,15,3},
 {"CORE CONTROL","Strong foundations, steady form","CORE","BEGINNER",{{0,10,8,11,9,32,24,23,0,0}},8,30,20,2},
 {"FULL BODY HIIT","Sweaty intervals, zero equipment","FULL BODY","INTERMEDIATE",{{0,2,3,14,17,5,31,16,23,0}},9,40,20,4},
 {"LEG DAY","Quads, glutes and balance","LEGS","INTERMEDIATE",{{0,2,5,12,13,9,33,28,22,0}},9,40,20,1},
 {"CHEST & CORE","Push, stabilize, recover","STRENGTH","INTERMEDIATE",{{1,3,7,4,26,31,10,23,0,0}},8,40,20,0},
 {"MORNING ENERGY","Wake up and move mindfully","MOBILITY","BEGINNER",{{0,1,25,29,21,15,28,22,0,0}},8,30,10,3},
 {"NO-JUMP SWEAT","Low-impact total body circuit","CARDIO","INTERMEDIATE",{{0,15,30,18,2,19,34,25,23,0}},9,35,15,3},
 {"BODYWEIGHT BURN","Challenging full-body rounds","HIIT","ADVANCED",{{0,14,3,17,33,16,26,31,23,0}},9,45,15,4},
 {"POSTURE RESET","Back, core and gentle mobility","RECOVERY","BEGINNER",{{0,21,27,11,20,28,25,35,23,0}},9,35,15,2},
 {"GLUTES & LEGS","Controlled strength for lower body","LEGS","BEGINNER",{{0,2,9,6,12,5,21,22,0,0}},8,35,20,5},
 {"QUICK 10","Short effective daily routine","FULL BODY","BEGINNER",{{0,15,2,4,10,19,9,23,0,0}},8,40,20,1}
};
inline constexpr int PLAN_COUNT=sizeof(plans)/sizeof(plans[0]);
inline int duration_seconds(const Plan& p){return p.count*p.seconds+(p.count-1)*p.rest;}
enum class Screen {Home,Workouts,Progress,Programs,Settings,Detail,Session,Completed,Exit};
struct Input {bool up=false,down=false,left=false,right=false,confirm=false,back=false,triangle=false,square=false,options=false,l1=false,r1=false;};
struct History { int completed=0, seconds=0, streak=0; long last_day=-1; std::array<int,7> week{};};
struct State {
 Screen screen=Screen::Home; Screen previous=Screen::Home; int nav=0,focus=0,selected=0,level=0,settings_focus=0;
 int exercise_index=0,completed_steps=0; bool resting=false,paused=false,dirty=false; int remaining_ms=0; History history;
 static long day_now(){return long(std::time(nullptr)/86400);}
 void start(int index){
  selected=std::clamp(index,0,PLAN_COUNT-1);screen=Screen::Session;
  exercise_index=0;completed_steps=0;resting=false;paused=false;remaining_ms=plans[selected].seconds*1000;
 }
 void finish(){
  const int sec=duration_seconds(plans[selected]);
  long day=day_now(); // UTC days. Documented; avoids timezone-dependent streak resets.
  if(history.last_day>=0&&day>history.last_day){
    long diff=day-history.last_day;
    if(diff>0){for(long i=0;i<diff&&i<7;i++)history.week[(history.last_day+1+i)%7]=0;}
    if(diff>1)history.streak=0;
  }
  if(day!=history.last_day)history.streak++;
  history.last_day=day;history.completed++;history.seconds+=sec;
  if(day>=0)history.week[day%7]+=sec;
  screen=Screen::Completed;dirty=true;paused=false;resting=false;remaining_ms=0;
 }
 void step_next(){
  const auto& p=plans[selected];
  if(!resting){if(exercise_index+1>=p.count){finish();return;}resting=true;remaining_ms=p.rest*1000;}
  else{resting=false;exercise_index++;completed_steps=exercise_index;remaining_ms=p.seconds*1000;}
 }
 void tick(int dt_ms){if(screen!=Screen::Session||paused)return;remaining_ms-=std::clamp(dt_ms,0,250);if(remaining_ms<=0)step_next();}
 // Returns true on user-confirmed exit.
 bool handle(const Input& in) {
  auto move=[&](int limit){if(in.left||in.up)focus=(focus+limit-1)%limit;if(in.right||in.down)focus=(focus+1)%limit;};
  if(in.l1&&screen!=Screen::Session) {nav=(nav+4)%5;screen=static_cast<Screen>(nav);focus=0;}
  if(in.r1&&screen!=Screen::Session) {nav=(nav+1)%5;screen=static_cast<Screen>(nav);focus=0;}
  switch(screen){
   case Screen::Home:
     if(in.left||in.right)move(4);
     if(in.up||in.down){nav=(nav+(in.down?1:4))%5;screen=static_cast<Screen>(nav);focus=0;}
     if(in.confirm){selected=focus;previous=Screen::Home;screen=Screen::Detail;}
     if(in.triangle)start(focus);
     if(in.back){screen=Screen::Exit;focus=1;}
     break;
   case Screen::Workouts:
     if(in.up||in.down)move(PLAN_COUNT);
     if(in.confirm){selected=focus;previous=Screen::Workouts;screen=Screen::Detail;}
     if(in.triangle)start(focus);
     if(in.back){screen=Screen::Home;nav=0;focus=0;}break;
   case Screen::Programs:
     if(in.up||in.down||in.left||in.right)move(3);
     if(in.confirm){selected=focus==0?0:focus==1?3:9;previous=Screen::Programs;screen=Screen::Detail;}
     if(in.back){screen=Screen::Home;nav=0;focus=0;}break;
   case Screen::Progress:
     if(in.confirm){screen=Screen::Workouts;nav=1;focus=0;}
     if(in.back){screen=Screen::Home;nav=0;}break;
   case Screen::Settings:
     if(in.up||in.down){settings_focus=(settings_focus+(in.down?1:2))%3;}
     if(in.confirm||in.left||in.right){if(settings_focus==0)level=(level+1)%3;else if(settings_focus==1){history=History{};dirty=true;}}
     if(in.back){screen=Screen::Home;nav=0;}break;
   case Screen::Detail:
     if(in.confirm)start(selected);
     if(in.left)selected=(selected+PLAN_COUNT-1)%PLAN_COUNT;
     if(in.right)selected=(selected+1)%PLAN_COUNT;
     if(in.back){screen=previous;focus=previous==Screen::Home?selected%4:selected;}break;
   case Screen::Session:
     if(in.confirm)paused=!paused;
     if(in.square)step_next();
     if(in.triangle){if(resting){resting=false;remaining_ms=plans[selected].seconds*1000;}else if(exercise_index>0){--exercise_index;completed_steps=exercise_index;remaining_ms=plans[selected].seconds*1000;}}
     if(in.back||in.options)paused=!paused;
     if(in.l1){screen=Screen::Detail;previous=Screen::Workouts;paused=false;}
     break;
   case Screen::Completed:
     if(in.confirm){screen=Screen::Home;nav=0;focus=0;}
     if(in.triangle)start(selected);
     if(in.back){screen=Screen::Home;nav=0;focus=0;}break;
   case Screen::Exit:
     if(in.left||in.right||in.up||in.down)focus=1-focus;
     if(in.confirm){if(focus==0)return true;screen=Screen::Home;focus=0;}
     if(in.back){screen=Screen::Home;focus=0;}
     break;
  }
  return false;
 }
};
}
