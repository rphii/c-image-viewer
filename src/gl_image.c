#include <stdbool.h>

#include "gl_image.h"
#include "uniform.h"

static bool gl_image_initialized;
static unsigned int VAO, VBO, EBO;

void gl_image_initialize(void) {

    float vertices[4*5] = {
        // positions          // texture coords
         1.0f,  1.0f,  0.0f,  0.0f,  0.0f, // top right
         1.0f, -1.0f,  0.0f,  0.0f,  1.0f, // bottom right
        -1.0f, -1.0f,  0.0f, -1.0f,  1.0f, // bottom left
        -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, // top left
    };
    unsigned int indices[6] = { // note that we start from 0!
        0, 1, 3, // first triangle
        1, 2, 3 // second triangle
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *)(sizeof(float) * 0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *)(sizeof(float) * 3));

    glBindVertexArray(0);
    gl_image_initialized = true;
}

void gl_image_shader(GlImage *image, Shader shader) {
    if(!gl_image_initialized) gl_image_initialize();

    image->loc_projection = get_uniform(shader, "projection");
    image->loc_view = get_uniform(shader, "view");
    image->loc_transform = get_uniform(shader, "transform");
    image->shader = shader;
}

void gl_image_render(GlImage *image, unsigned int texture, mat4 projection, mat4 view, mat4 transform) {
    if(!gl_image_initialized) gl_image_initialize();

    glUseProgram(image->shader);

    glBindVertexArray(VAO);
    glUniformMatrix4fv(image->loc_projection, 1, GL_FALSE, (float *)projection);
    glUniformMatrix4fv(image->loc_view, 1, GL_FALSE, (float *)view);
    glUniformMatrix4fv(image->loc_transform, 1, GL_FALSE, (float *)transform);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

