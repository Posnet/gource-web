#version 300 es
precision highp float;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_texcoord;

uniform mat4 u_mvp;

out vec4 v_color;
out vec2 v_texcoord;

void main() {
    v_color = a_color;
    v_texcoord = a_texcoord;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
