#version 300 es
precision highp float;

in vec4 v_color;
in vec2 v_texcoord;

uniform sampler2D u_texture;
uniform float u_shadow_strength;

out vec4 fragColor;

void main() {
    vec4 colour = texture(u_texture, v_texcoord);
    fragColor = vec4(0.0, 0.0, 0.0, v_color.a * colour.a * u_shadow_strength);
}
