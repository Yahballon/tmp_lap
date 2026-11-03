// Host regression tests for AndroidFlow virtual touchscreen controller.
#include "../native/yah_godot_cursor.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void) {
    yah_godot_cursor p = { .x = 960, .y = 540 };
    assert(!yah_pointer_tick(&p, 1.f / 60));
    assert(p.x == 960 && p.y == 540);
    p.dx = 0.10f;
    assert(!yah_pointer_tick(&p, 1.f / 60));
    p.dx = 1.f;
    assert(yah_pointer_tick(&p, 1.f / 60));
    assert(p.x > 960.f && p.x <= 1919.f && p.y == 540.f);
    p.x = 1918.f;
    yah_pointer_tick(&p, 1.f);
    assert(p.x == 1919.f);
    p.dx = -1.f;
    for (int i=0; i<500; i++) yah_pointer_tick(&p, 1.f/60.f);
    assert(p.x == 0.f);
    p.dx = 0;
    p.dy = 1;
    for (int i=0; i<500; i++) yah_pointer_tick(&p, 1.f/60.f);
    assert(p.y == 1079.f);
    assert(yah_pointer_coord(-0.5f, 1920) == 0);
    assert(yah_pointer_coord(0.5f, 1920) == 960);
    assert(yah_pointer_coord(1.5f, 1920) == 1919);
    assert(yah_pointer_coord(0.5f, 1080) == 540);
    puts("PASS Godot DualSense cursor, deadzone, frame-time and touch coordinates");
    return 0;
}
