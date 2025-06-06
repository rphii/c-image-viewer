#include "civ_config.h"
#include "civ.h"
#include <rphii/file.h>

#include <unistd.h>
#include <wordexp.h>
#include <sys/sysmacros.h>

ErrDecl civ_config_load(Civ *civ, Str *path);
ErrDecl civ_config_path(Civ *civ, Str *path);

#define ERR_civ_config_load(civ, path) "error while loading config " F("'%.*s'", BOLD), STR_F(*path)
ErrImpl civ_config_load(Civ *civ, Str *path) {
    ASSERT_ARG(civ);
    ASSERT_ARG(path);
    if(!str_len(*path)) return 0;
    Str *content = &civ->config_content;
    str_pdyn(content);
    TRYC(file_str_read(*path, content));
    arg_config(civ->arg, *content);
    return 0;
error:
    return -1;
}

#define ERR_civ_config_path(...) "error while getting config path"
ErrImpl civ_config_path(Civ *civ, Str *path) {
    ASSERT_ARG(civ);
    ASSERT_ARG(path);
    int err = 0;
    const char *paths[] = {
        "$XDG_CONFIG_HOME/imv/civ.conf",
        "$HOME/.config/civ/civ.conf",
        "/etc/civ/civ.conf"
    };
    char cpath[FILE_PATH_MAX];
    wordexp_t word = {0};
    for(size_t i = 0; i < SIZE_ARRAY(paths); ++i, str_clear(path)) {
        wordfree(&word);
        memset(&word, 0, sizeof(word));
        if(wordexp(paths[i], &word, 0)) {
            continue;
        }
        if(!word.we_wordv[0]) {
            continue;
        }
        str_fmt(path, "%s", word.we_wordv[0]);
        str_as_cstr(*path, cpath, FILE_PATH_MAX);
        if(!strlen(cpath) || access(cpath, R_OK) == -1) {
            continue;
        }
        break;
    }
clean:
    wordfree(&word);
    return err;
error:
    ERR_CLEAN;
}

ErrImpl civ_config_defaults(Civ *civ) {
    int err = 0;
    CivConfig *defaults = &civ->defaults;
    defaults->font_path = str("/usr/share/fonts/MonoLisa/ttf/MonoLisa-Regular.ttf");
    defaults->font_size = 18;
    defaults->show_description = false;
    defaults->show_loaded = true;
    defaults->jobs = sysconf(_SC_NPROCESSORS_ONLN);
    defaults->shuffle = false;
    defaults->image_cap = 0;

    /* finally, do defaults here */
    civ->config = *defaults;

    /* load config */
    Str path = STR_DYN();
    TRYC(civ_config_path(civ, &path));
    TRYC(civ_config_load(civ, &path));
    //printff("PATH: '%.*s'", STR_F(path));

clean:
    str_free(&path);
    return err;
error:
    ERR_CLEAN;
}



