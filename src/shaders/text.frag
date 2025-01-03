#version 330 core
out vec4 color;

in VS_OUT {
    vec2 tex_coords;
    flat int index;
} fs_in;

uniform sampler2DArray text;
uniform int letter_map[250];
uniform vec3 text_color;

void main() {
    float alpha = texture(text, vec3(fs_in.tex_coords.xy, letter_map[fs_in.index])).r;
    //alpha = (alpha + 0.05) / 1.05;
    vec4 sampled = vec4(1.0, 1.0, 1.0, alpha);
    color = vec4(text_color, 1.0) * sampled;
}

