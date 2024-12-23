#include "shader.h"
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
    printf("read %zu len %zu\n", read, len);
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
        printf("SHADER COMPILE ERROR:\n%*s\n", (int)sizeof(info), info);
        assert(0);
    }
}


static unsigned int static_shader_load(unsigned int type, const char *dir, const char *file) {
    char path[4096];
    snprintf(path, 4096, "%s/%s", dir, file);
    const char *src = static_file_read(path);
    printf("[SHADER] Load '%s'\n", path);
    /* compile */
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, 0);
    glCompileShader(shader);
    static_shader_assert(shader, src);
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

Shader shader_load(const char *dir_v, const char *vertex, const char *dir_f, const char *fragment) {

    Shader shader_vertex = static_shader_load(GL_VERTEX_SHADER, dir_v, vertex);
    Shader shader_fragment = static_shader_load(GL_FRAGMENT_SHADER, dir_f, fragment);

    Shader shader = glCreateProgram();
    glAttachShader(shader, shader_vertex);
    glAttachShader(shader, shader_fragment);
    glLinkProgram(shader);
    static_program_assert(shader);

    glDeleteShader(shader_vertex);
    glDeleteShader(shader_fragment);

    return shader;
}

void shader_free(Shader shader) {
    glDeleteProgram(shader);
}

