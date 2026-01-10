#version 300 es
precision highp float;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec4 a_texcoord;  // xy = texcoord, zw = center offset

uniform mat4 u_mvp;

out vec4 v_color;
out vec2 v_texcoord;
out vec3 v_pos;

void main() {
    v_pos = a_position - a_texcoord.yzw;
    v_texcoord = a_texcoord.xy;
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
