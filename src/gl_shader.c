#include "gl_shader.h"
#include <stdio.h>
#include <stdlib.h>

static char *static_file_read(const char *filename) {
    printf("[FILE] Read '%s'\n", filename);
    FILE *fp = fopen(filename, "r");
    if(!fp) assert(0 && "failed loading file!");
    fseek(fp, 0L, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char *str = malloc(len + 1);
    assert(str && "failed allocation!");
    size_t read = fread(str, 1, len, fp);
    fclose(fp);
    assert(read == len);
    str[len] = 0;
    return str;
}

static void static_shader_assert(int shader, const char *src) {
    /* verify */
    int success = 0;
    char info[4096] = {0};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(shader, sizeof(info), 0, info);
        printf("SHADER COMPILE ERROR:\n%.*s\n", (int)sizeof(info), info);
        assert(0);
    }
}

static unsigned int static_shader_load(unsigned int type, const char *src, int length) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, &length);
    glCompileShader(shader);
    static_shader_assert(shader, src);
    return shader;
}

static unsigned int static_shader_load_dir(unsigned int type, const char *dir, const char *file) {
    char path[4096];
    snprintf(path, 4096, "%s/%s", dir, file);
    const char *src = static_file_read(path);
    printf("[SHADER] Load '%s'\n", path);
    /* compile */
    unsigned int shader = static_shader_load(type, src, 0);
    free((char *)src);
    return shader;
}


static void static_program_assert(unsigned int program) {
    /* verify */
    int success = 0;
    char info[4012] = {0};
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(program, sizeof(info), 0, info);
        printf("SHADER PROGRAM ERROR:\n%*s\n", (int)sizeof(info), info);
        assert(0);
    }
}

Shader shader_load(const char *vert, int vert_len, const char *frag, int frag_len) {
    Shader shader_vertex = { .id = static_shader_load(GL_VERTEX_SHADER, vert, vert_len), .loaded = true };
    Shader shader_fragment = { .id = static_shader_load(GL_FRAGMENT_SHADER, frag, frag_len), .loaded = true };

    Shader shader = { .id = glCreateProgram(), .loaded = true };
    glAttachShader(shader.id, shader_vertex.id);
    glAttachShader(shader.id, shader_fragment.id);
    glLinkProgram(shader.id);
    static_program_assert(shader.id);

    glDeleteShader(shader_vertex.id);
    glDeleteShader(shader_fragment.id);

    return shader;
}

Shader shader_load_dir(const char *dir_v, const char *vertex, const char *dir_f, const char *fragment) {

    Shader shader_vertex = { .id = static_shader_load_dir(GL_VERTEX_SHADER, dir_v, vertex), .loaded = true };
    Shader shader_fragment = { .id = static_shader_load_dir(GL_FRAGMENT_SHADER, dir_f, fragment), .loaded = true };

    Shader shader = { .id = glCreateProgram(), .loaded = true };
    glAttachShader(shader.id, shader_vertex.id);
    glAttachShader(shader.id, shader_fragment.id);
    glLinkProgram(shader.id);
    static_program_assert(shader.id);

    glDeleteShader(shader_vertex.id);
    glDeleteShader(shader_fragment.id);

    return shader;
}

void shader_free(Shader shader) {
    if(shader.loaded) {
        glDeleteProgram(shader.id);
        shader.loaded = false;
    }
}

