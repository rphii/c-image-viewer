#ifndef SHADER_H

#include <assert.h>
#include "glad.h"

typedef unsigned int Shader;

Shader shader_load(const char *dir_v, const char *vertex, const char *dir_f, const char *fragment);
void shader_free(Shader shader);


#define SHADER_H
#endif

