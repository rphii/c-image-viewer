#ifndef IMAGE_H

#include <cglm/cglm.h>
#include "shader.h"

void image_shader(Shader shader);
void image_render(unsigned int texture, mat4 projectino, mat4 view, mat4 transform);

#define IMAGE_H
#endif

