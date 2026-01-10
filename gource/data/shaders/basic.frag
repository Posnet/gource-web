#version 300 es
precision highp float;

in vec4 v_color;
in vec2 v_texcoord;

uniform sampler2D u_texture;
uniform bool u_use_texture;

out vec4 fragColor;

void main() {
    if (u_use_texture) {
        fragColor = texture(u_texture, v_texcoord) * v_color;
    } else {
        fragColor = v_color;
    }
}
