#version 330

uniform float time;
uniform vec2 resolution;
uniform sampler2D tex;

in vec2 tex_coord;
out vec4 frag_color;

void main() {
    vec2 uv = tex_coord * 2.0 - 1.0;
    vec2 p = -1.0 + 2.0 * tex_coord;
    float a = time * 3.0;
    float d, e = 100.0;
    vec3 c = vec3(0.0);
    vec3 b = c + 1.0;
    for(int j = 0; j < 99; j++) {
        d = length(p);
        p += vec2(cos(a) + sin(d * e - a), sin(a) - cos(d * e - a)) / d * 0.5;
        float f = sin(d * 9.0 - a);
        c += b * f;
        a += 0.05;
    }
    c = mix(c, vec3(sin(a), sin(a + 2.0), sin(a + 4.0)), 0.5); // Add colorful noise
    vec4 base_color = texture(tex, tex_coord);
    frag_color = base_color + vec4(c * 0.5, 0.0);
}
