// SPDX-License-Identifier: GPL-3.0-or-later
// Starbound Sprint - original side-scrolling platform game.
#pragma once
#include <array>
#include <vector>

namespace sprint {
constexpr int kTile = 60;
constexpr int kRows = 13;
constexpr int kColumns = 120;
constexpr int kStages = 8;
enum class Phase { menu, playing, paused, stage_clear, game_over, victory };
struct Vec2 { float x=0.f, y=0.f; };
struct Pickup { Vec2 pos; bool taken=false; };
struct Drone { Vec2 pos; float direction=1.f; bool alive=true; };
struct Control { float horizontal=0.f; bool jump=false; bool sprint=false; };
struct Progress {
    int unlocked=0;
    int best_score=0;
    bool reduced_motion=false;
    void load();
    bool save() const;
};
class Game {
public:
    Phase phase=Phase::menu;
    int stage=0, lives=3, score=0, crystals=0, invulnerable=0;
    float elapsed=0.f, camera=0.f;
    Vec2 player;
    Vec2 velocity;
    bool grounded=false, facing_right=true;
    int jumps_remaining() const { return 2 - jumps_used_; }
    std::array<std::array<char,kColumns>,kRows> tiles{};
    std::vector<Pickup> pickups;
    std::vector<Drone> drones;
    Progress progress;
    Game();
    void new_run(int first_stage);
    void step(const Control& controls);
    void next_stage();
    void set_pause(bool paused);
    void reset_to_menu();
    bool solid_at(int col,int row) const;
    float finish_x() const { return static_cast<float>((kColumns-5)*kTile); }
    static const char* stage_name(int index);
private:
    float checkpoint_x=0.f;
    bool jump_was_held=false;
    int jumps_used_=0;  // Cleared only on landing, respawn or entering a world.
    void make_stage();
    void respawn();
    void damage();
    void move_x(float dx);
    void move_y(float dy);
};
} // namespace sprint
