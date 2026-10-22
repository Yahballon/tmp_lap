#!/usr/bin/env python3
"""End-to-end assertions on the preview cache built from genuine Open Surge."""
from pathlib import Path
import json
import sys

app=Path(sys.argv[1] if len(sys.argv)>1 else "app")
root=app/"assets"/"level-preview"
header=(root/"header.txt").read_text().splitlines()
meta={row.split()[0]:tuple(map(int,row.split()[1:])) for row in header}
assert set(meta)=={"atlas","hero","spawn","instances"}
defs=[row.split() for row in (root/"defs.txt").read_text().splitlines()]
instances=[row.split() for row in (root/"bricks.txt").read_text().splitlines()]
assert len(defs)>=90
assert len(instances)==meta["instances"][0]
assert len(instances)>=1000
assert all(len(item)==4 and item[3] in {"default","green","yellow"}
           for item in instances)
layered=[item for item in instances if item[3]!="default"]
assert len(layered)==11, layered
assert sum(item[3]=="green" for item in layered)==7
assert sum(item[3]=="yellow" for item in layered)==4
routes=[row.split() for row in (root/"routes.tsv").read_text().splitlines()]
route_kinds=[row[0] for row in routes]
assert route_kinds.count("tube_in")==6 and route_kinds.count("tube_out")==6
assert route_kinds.count("green")>=20 and route_kinds.count("yellow")>=20
assert meta["spawn"]==(176,9792), "Pinned original demo spawn changed"
for filename,dimension in (("sunshine.rgba",meta["atlas"]),("surge.rgba",meta["hero"])):
    assert (root/filename).stat().st_size==dimension[0]*dimension[1]*4
    pixels=(root/filename).read_bytes()
    # Original magenta transparency must be zero alpha, not hot-pink blocks.
    assert not any(pixels[i:i+3]==bytes((255,0,255)) and pixels[i+3]!=0
                   for i in range(0,len(pixels),4)), filename
collision=(root/"collision.bin").read_bytes()
assert len(collision)==meta["atlas"][0]*meta["atlas"][1]
assert all(pixel in (0,1) for pixel in collision)
assert 1000<sum(collision)<len(collision)*.95
by_id={int(row[0]):row for row in defs}
assert len(by_id)==len(defs)
assert {row[5] for row in defs}>={"SOLID","CLOUD","PASSABLE"}
assert all(int(row[0]) in by_id for row in instances)
assert all(0<=int(row[1])<=int(row[1])+int(row[3])<=meta["atlas"][0]
           and 0<=int(row[2])<=int(row[2])+int(row[4])<=meta["atlas"][1]
           for row in defs)
manifest=json.loads((app/"assets"/"game-data-manifest.json").read_text())
assert manifest["file_count"]>100
# Regression: DrawList.image UV Rect is x,y,width,height, never x0,y0,x1,y1.
native=(Path(__file__).resolve().parents[1]/"native"/"level_preview.hpp").read_text()
assert "{u0,v0,u1-u0,v1-v0}" in native
assert "facing_<0?u0-u1:u1-u0,v1-v0" in native
assert "mask_pixel(b,int(px-float(b.x)),int(py-float(b.y)))" in native
assert "floor_pixel(sensor,at)" in native
native_cpp=(Path(__file__).resolve().parents[1]/"native"/"level_preview.hpp").read_text()
main_cpp=(Path(__file__).resolve().parents[1]/"native"/"main.cpp").read_text()
assert "bool build_grid()" in native_cpp
assert "brick_in_layer(" in native_cpp
assert "tick_routes()" in native_cpp
assert "tick_tube(" in native_cpp
assert "ORIGINAL_TUBE_IN_SCRIPT" in main_cpp
assert "std::lower_bound(cells_.begin()" in native_cpp
assert "1.f/120.f" in main_cpp
assert "frame_elapsed_us<16667u" in main_cpp
assert "PERF fps=" in main_cpp
assert "bool find_floor(" in native_cpp
assert "TERRAIN_BLOCK" in main_cpp
assert "auto_respawns()" in native_cpp
pad_cpp=(Path(__file__).resolve().parents[1]/"native"/"pad_state.hpp").read_text()
assert "if(count==0)" in pad_cpp
assert "frame.held=held_" in pad_cpp
assert "frame.pressed |= bits & ~held_" in pad_cpp
assert "pad_state.update(samples.data(),count)" in main_cpp
assert "PAD_DIAG" in main_cpp
# Confirm native entity positions are sourced from the real upstream level.
entities=(root/"entities.tsv").read_text().splitlines()
kinds=[line.split("\t",1)[0] for line in entities]
assert len(entities)==128 and kinds.count("ring")==124
assert kinds.count("spring")==1 and kinds.count("spring_stronger")==2
assert kinds.count("goal")==1
for name in ("collectibles","springs","goal"):
    w,h=map(int,(root/(name+".txt")).read_text().split())
    assert 16<=w<=4096 and 16<=h<=4096
    assert (root/(name+".rgba")).stat().st_size==w*h*4
native_moves=(Path(__file__).resolve().parents[1]/"native"/"level_preview.hpp").read_text()
main_moves=(Path(__file__).resolve().parents[1]/"native"/"main.cpp").read_text()
assert "bool charging() const" in native_moves
assert "bool rolling() const" in native_moves
assert "480.f+240.f*charge_" in native_moves
assert "DOWN+X" in main_moves
assert "MOVE_SPINDASH_RELEASE" in main_moves
assert "sprite_frame()" in native_moves
entities_cpp=(Path(__file__).resolve().parents[1]/"native"/"level_entities.hpp").read_text()
assert "spring_armed" in entities_cpp and "spring_cooldown" in entities_cpp
hazards=(root/"hazards.tsv").read_text().splitlines()
hazard_kinds=[row.split("\t")[0] for row in hazards]
assert len(hazards)==13
assert hazard_kinds.count("mosquito")==5
assert hazard_kinds.count("wolfey")==1
assert hazard_kinds.count("bugsy")==1
assert hazard_kinds.count("spikes")==6
for name in ("mosquito","wolfey","bugsy","spikes"):
    w,h=map(int,(root/(name+".txt")).read_text().split())
    assert 32<=w<=4096 and 32<=h<=4096
    assert (root/(name+".rgba")).stat().st_size==w*h*4
combat=(Path(__file__).resolve().parents[1]/"native"/"level_hazards.hpp").read_text()
assert "bool game_over()" in combat and "void restart_level()" in combat
assert "float invulnerability()" in combat
assert "ENEMY_DEFEATED" in main_cpp and "PLAYER_HIT_RING_LOSS" in main_cpp
audio=(app/"assets"/"audio-cache"/"manifest.json")
assert audio.exists(), "Original music and sound manifest missing"
audio_manifest=json.loads(audio.read_text())
assert audio_manifest["sample_rate"]==48000
assert len(audio_manifest["sfx"])>=16
assert (app/"assets"/"audio-cache"/"level.ogg").is_file()
uphill_cpp=(Path(__file__).resolve().parents[1]/"native"/"level_preview.hpp").read_text()
assert "floor_height_at(" in uphill_cpp
assert "rolling_ || charging_" in uphill_cpp
assert "feet-25.f" in uphill_cpp and "px+7.f" in uphill_cpp
assert "rolluphillslp" in uphill_cpp or "281.25f" in uphill_cpp
assert "uphill_steps_" in uphill_cpp
assert "wall_climb_" in uphill_cpp
assert "never bypass walls" not in uphill_cpp or "rolling" in uphill_cpp
native_src=(Path(__file__).resolve().parents[1]/"native"/"level_preview.hpp").read_text()
assert "bool tube_exit_usable(" in native_src
assert "restore_safe_ground()" in native_src
assert "loop_safety_recoveries_" in native_src
print("PASS original demo terrain/sprites staged for PS5: "
      f"{len(instances)} bricks, {len(defs)} types, "
      f"{meta['atlas']} RGBA atlas, real player sprite; NOT SurgeScript gameplay")
