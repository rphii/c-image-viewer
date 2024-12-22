#version 330 core
layout (location = 0) in vec2 in_vertex; // <vec2 pos>

out VS_OUT {
    vec2 tex_coords;
    flat int index;
} vs_out;

uniform mat4 transforms[250];    // try UBO, SSBO... to avoid running out of array space
uniform mat4 projection;

void main() {
    gl_Position = projection * transforms[gl_InstanceID] * vec4(in_vertex.xy, 0.0, 1.0);
    vs_out.index = gl_InstanceID;
    vs_out.tex_coords = in_vertex.xy;
    vs_out.tex_coords.y = 1.0 - vs_out.tex_coords.y;
}

