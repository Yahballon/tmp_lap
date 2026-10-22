# Open Surge PS5 v0.6.2 — S-shaped tunnel and looping route fix

Real PS5 gameplay video v0.6.1: Surge clips straight through Sunshine S-curves and drops out of the loop before reaching its ceiling.

- Only demo-1 authored brick placements (pairs 50/51 and 53/52, original loop 41/42) activate these custom native preview paths.
- Snake tunnel follows a continuous 256px wavelength curve with 46px amplitude, 120Hz arclength stepping and retained Spin Dash speed.
- The circle is followed for one complete revolution (bottom → right → top → left → bottom) and exits right; reversed horizontal velocity during the ceiling arc cannot prematurely terminate the loop.
- No walking teleport: activation only when rolling quickly into the curve entrance.
- Native regression performs the entire snake and 360-degree loop and checks vertical travel, reversed direction, early exits, and reset.
- Original music, UI, stages, saves, enemies and controller pipeline unchanged.

**Important:** This is a PS5-native approximation that uses geometry-anchored authored curves instead of original 4-mode source engine physics. The stage has many other obstacles that still need proper MM_FLOOR/MM_WALL/MM_CEILING handling. Hardware playtest required.
