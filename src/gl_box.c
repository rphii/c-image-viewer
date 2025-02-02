#include "gl_box.h"

static bool box_initialized;
static GLuint VAO, VBO, EBO;

void box_init(void) {

    float box[] = {
        // positions        
        1.0f,  1.0f,  0.0f,
        1.0f,  0.0f,  0.0f,
        0.0f,  0.0f,  0.0f,
        0.0f,  1.0f,  0.0f,
    };

    unsigned int indices[] = { // note that we start from 0!
        0, 1, 3, // first triangle
        1, 2, 3 // second triangle
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(box), &box, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *)(sizeof(float) * 0));
    box_initialized = true;
}

void box_shader(Shader shader) {
}

void box_render(Shader shader, mat4 projection, vec4 dim, vec4 color, float border) {

    if(!box_initialized) box_init();

    /* draw a box under the text */
    mat4 m_scale;
    mat4 m_transform;
    mat4 m_pos;
    glm_mat4_identity(m_transform);
    glm_mat4_identity(m_pos);
    glm_mat4_identity(m_scale);

    int loc_box_transform = glGetUniformLocation(shader.id, "transform");
    int loc_box_projection = glGetUniformLocation(shader.id, "projection");
    int loc_box_color = glGetUniformLocation(shader.id, "color");
    assert(loc_box_transform >= 0);
    assert(loc_box_projection >= 0);
    assert(loc_box_color >= 0);

    float sx = (float)(dim[2] - dim[0]);
    float sy = (float)(dim[3] - dim[1]);
    float tx = dim[0];
    float ty = dim[1];

    glm_scale(m_scale, (vec3){ sx + border, sy + border });
    glm_translate(m_transform, (vec3){ tx - border / 2, ty - border / 2 });
    glm_mat4_mul(m_transform, m_scale, m_transform);

#if 0
    glBlendFunc(GL_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

    glUseProgram(shader.id);
    glBindVertexArray(VAO);
    glUniformMatrix4fv(loc_box_transform, 1, GL_FALSE, (float *)m_transform);
    glUniformMatrix4fv(loc_box_projection, 1, GL_FALSE, (float *)projection);
    glUniform4f(loc_box_color, color[0], color[1], color[2], color[3]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

