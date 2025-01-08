#ifndef CIV_CONFIG_H

#include "str.h"

struct Civ;

typedef struct CivConfig {
    RStr font_path;
    ssize_t font_size;
    bool show_description;
    bool show_loaded;
    ssize_t jobs;
    bool qafl;
    bool shuffle;
    ssize_t image_cap;
} CivConfig;

typedef enum {
    CIV_NONE,
    /* below */
    CIV_FONT_PATH,
    CIV_FONT_SIZE,
    CIV_SHOW_DESCRIPTION,
    CIV_SHOW_LOADED,
    CIV_JOBS,
    CIV_QAFL,
    CIV_SHUFFLE,
    CIV_IMAGE_CAP,
    /* above */
    CIV__COUNT
} CivConfigList;

#define ERR_civ_config_defaults(...) "error while loading config defaults"
ErrDecl civ_config_defaults(struct Civ *civ);

#define CIV_CONFIG_H
#endif

