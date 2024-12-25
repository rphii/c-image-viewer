#ifndef GL_IMAGE_H

#include "shader.h"
#include <cglm/cglm.h>

typedef struct GlImage {
    int loc_projection;
    int loc_view;
    int loc_transform;
} GlImage;

void gl_image_shader(GlImage *image, Shader shader);
void gl_image_render(GlImage *image, unsigned int texture, mat4 projection, mat4 view, mat4 transform);

#define GL_IMAGE_H
#endif

