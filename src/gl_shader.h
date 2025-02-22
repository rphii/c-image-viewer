#ifndef SHADER_H

#include <stdbool.h>
#include <assert.h>
#include "glad.h"

typedef struct Shader {
    unsigned int id;
    bool loaded;
} Shader;

Shader shader_load(const char *vert, int vert_len, const char *frag, int frag_len);
Shader shader_load_dir(const char *dir_v, const char *vertex, const char *dir_f, const char *fragment);
void shader_free(Shader shader);


#define SHADER_H
#endif

