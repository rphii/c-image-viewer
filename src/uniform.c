#include "uniform.h"

#include <stdio.h>
#include <assert.h>

int get_uniform(int program, const char *uniform) {
    printf("[UNIFORM] Locating '%s'\n", uniform);
    int result = glGetUniformLocation(program, uniform);
    assert(result >= 0);
    return result;
}

