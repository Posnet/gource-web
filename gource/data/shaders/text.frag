#version 300 es
precision highp float;

in vec4 v_color;
in vec2 v_texcoord;

uniform sampler2D u_texture;
uniform float u_shadow_strength;
uniform float u_texel_size;

out vec4 fragColor;

void main() {
    // Font texture uses GL_RED format - sample from red channel for glyph alpha
    float colour_alpha = texture(u_texture, v_texcoord).r;
    float shadow_alpha = texture(u_texture, v_texcoord - vec2(u_texel_size)).r * u_shadow_strength;

    float combined_alpha = 1.0 - (1.0 - shadow_alpha) * (1.0 - colour_alpha);

    // Blend between shadow (black) and text color based on colour_alpha ratio
    vec3 final_color = v_color.rgb;
    float final_alpha = v_color.a * combined_alpha;

    // If combined_alpha is 0, the pixel is fully transparent
    if (combined_alpha < 0.001) {
        discard;
    }

    fragColor = vec4(final_color, final_alpha);
}
