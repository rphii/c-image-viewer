#ifndef BOX_H

#include <cglm/cglm.h>
#include <stdbool.h>
#include "shader.h"
#include "glad.h"

void box_render(Shader shader, mat4 projection, vec4 dim, vec4 color, float border);

#define BOX_H
#endif


