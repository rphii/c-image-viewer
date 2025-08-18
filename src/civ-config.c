#include "civ-config.h"
#include "civ.h"

#include <unistd.h>
#include <wordexp.h>
#include <sys/sysmacros.h>

ErrImpl civ_config_defaults(Civ *civ) {
    int err = 0;
    Civ_Config *defaults = &civ->defaults;
    defaults->font_path = so("/usr/share/fonts/MonoLisa/ttf/MonoLisa-Regular.ttf");
    defaults->font_size = 18;
    defaults->show_description = false;
    defaults->show_loaded = true;
    defaults->jobs = sysconf(_SC_NPROCESSORS_ONLN);
    defaults->shuffle = false;
    defaults->image_cap = 0;
    defaults->preview_load = true;

    /* finally, do defaults here */
    civ->config = *defaults;

    /* load config */
    So path = SO;
    //printff("PATH: '%.*s'", SO_F(path));

clean:
    so_free(&path);
    return err;
error:
    ERR_CLEAN;
}



