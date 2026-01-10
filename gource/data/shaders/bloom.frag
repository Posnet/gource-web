#version 300 es
precision highp float;

in vec4 v_color;
in vec2 v_texcoord;
in vec3 v_pos;

out vec4 fragColor;

void main() {
    float r = fract(sin(dot(v_pos.xy, vec2(11.3713, 67.3219))) * 2351.3718);
    float radius = v_texcoord.x;

    float offset = (0.5 - r) * radius * 0.045;
    float intensity = min(1.0, cos((length(v_pos * 2.0) + offset) / radius));
    float gradient = intensity * smoothstep(0.0, 2.0, intensity);

    gradient *= smoothstep(1.0, 0.67 + r * 0.33, 1.0 - intensity);

    fragColor = v_color * gradient;
}
