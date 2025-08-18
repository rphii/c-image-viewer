#ifndef CIV_CONFIG_H

#include <rlso.h>

struct Civ;

typedef struct Civ_Config {
    So font_path;
    ssize_t font_size;
    bool show_description;
    bool show_loaded;
    ssize_t jobs;
    bool qafl;
    bool shuffle;
    bool preview_load;
    bool preview_retain;
    bool pipe_and_args;
    ssize_t image_cap;
} Civ_Config;

#define ERR_civ_config_defaults(...) "error while loading config defaults"
ErrDecl civ_config_defaults(struct Civ *civ);

#define CIV_CONFIG_H
#endif

