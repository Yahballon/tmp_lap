#version 450
// PS5MesenX: dedicated video sprite shader, no SDF/rounded-rectangle branches.
// SPDX-License-Identifier: GPL-3.0-or-later
layout(set = 1, binding = 0) uniform sampler2D u_game;
layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 2) in vec4 v_params;
layout(location = 0) out vec4 o_color;
void main() {
    vec4 pixel = texture(u_game, v_uv);
    o_color = vec4(pixel.rgb, 1.0);
}
