#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 tex_coord;    

uniform mat4 view;
uniform mat4 projection;
uniform mat4 transform;

void main() {
    gl_Position = projection * view * transform * vec4(pos, 1.0);
    // transform  : correct initial scaling of image
    // view       : pan + zoom around
    // projection : undo of initial scaling
    tex_coord = tex;
}
