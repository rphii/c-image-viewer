#ifndef BOX_H

#include <cglm/cglm.h>
#include <stdbool.h>
#include "shader.h"
#include "glad.h"
#include "uniform.h"

void box_render(Shader shader, mat4 projection, vec4 dim, vec4 color);

#define BOX_H
#endif


