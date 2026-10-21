# Open Surge PS5 v0.4.0 — native enemy and hazard compatibility preview

Based on PS5-tested v0.3.1 with stable original movement, Spin Dash, rings and springs.

- Stage 13 original demo enemy/hazard positions and 4 authentic sprite atlases.
- Mosquito (5) flies left at 60 world units/s, Wolfey (1) bounded patrol at 80, Lady Bugsy (1) bounded patrol at 30 and simplified bullets at 60, Spikes (6) hurt the player.
- Rolling or descending onto enemies defeats them, +100 score per enemy.
- Damage gives 1.6s invulnerability. Lose collected rings first; otherwise lose a life and respawn. 3 lives and Game Over / Square restart HUD.
- All native C++ behavior, art size and original placement checked in CI. Previous DualSense/movement/mask collision tests retained.
- This is **NOT** the complete Open Surge engine. Full SurgeScript/Allegro, authentic enemy platformer, sound/music, multi-level stage flow, scatter rings and savegames remain to be ported.

Console test required: confirm sprites, spikes, death/restart, and preserved movement at ~60 FPS.
