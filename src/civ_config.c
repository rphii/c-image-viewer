#include "civ_config.h"
#include "civ.h"
#include <rphii/file.h>

#include <unistd.h>
#include <wordexp.h>
#include <sys/sysmacros.h>

RStr civ_config_list(CivConfigList id) {
    switch(id) {
        case CIV_FONT_PATH: return RSTR("font-path");
        case CIV_FONT_SIZE: return RSTR("font-size");
        case CIV_SHOW_DESCRIPTION: return RSTR("show-description");
        case CIV_SHOW_LOADED: return RSTR("show-loaded");
        case CIV_JOBS: return RSTR("jobs");
        case CIV_QAFL: return RSTR("quit-after-full-load");
        case CIV_SHUFFLE: return RSTR("shuffle");
        case CIV_IMAGE_CAP: return RSTR("image-cap");
        default: ABORT("unsupported id: '%u'", id);
    }
}

#define ERR_civ_config_load(...) "error while loading config"
[[nodiscard]] int civ_config_load(Civ *civ, Str *path) {
    ASSERT_ARG(civ);
    ASSERT_ARG(path);
    if(!str_length(*path)) return 0;
    size_t linenb = 1;
    Str *content = &civ->config_content;
    TRYC(file_str_read(str_rstr(*path), content));
    for(RStr line = {0}; line.first < content->last; line.s ? ++linenb : linenb, line = str_splice(*content, &line, '\n'), line = rstr_trim(line)) {
        if(!rstr_length(line) || !line.s) continue;
        if(rstr_get_front(&line) == '#') continue;
        size_t argnb = 0;
        //printff(">> LINE %zu : '%.*s'", linenb, RSTR_F(line));
        CivConfigList id = CIV_NONE;
        for(RStr arg = {0}; arg.first < line.last; arg.s ? ++argnb : argnb, arg = rstr_splice(line, &arg, '='), arg = rstr_trim(arg)) {
            if(!arg.s) continue;
            //printff(">> ARG %zu : '%.*s'", argnb, RSTR_F(arg));
            if(argnb == 0) {
                /* check which id we could possibly deal with */
                for(size_t j = 1; j < CIV__COUNT; ++j) {
                    RStr cmp = civ_config_list(j);
                    if(rstr_cmp(cmp, arg)) continue;
                    id = j;
                    break;
                }
            } else if(argnb == 1) {
                switch(id) {
                    case CIV_FONT_PATH: {
                        civ->defaults.font_path = arg;
                    } break;
                    case CIV_FONT_SIZE: {
                        TRYC(rstr_as_int(arg, &civ->defaults.font_size));
                    } break;
                    case CIV_SHOW_DESCRIPTION: {
                        TRYC(rstr_as_bool(arg, &civ->defaults.show_description, true));
                    } break;
                    case CIV_SHOW_LOADED: {
                        TRYC(rstr_as_bool(arg, &civ->defaults.show_loaded, true));
                    } break;
                    case CIV_JOBS: {
                        TRYC(rstr_as_int(arg, &civ->defaults.jobs));
                    } break;
                    case CIV_QAFL: {
                        TRYC(rstr_as_bool(arg, &civ->defaults.qafl, true));
                    } break;
                    case CIV_SHUFFLE: {
                        TRYC(rstr_as_bool(arg, &civ->defaults.shuffle, true));
                    } break;
                    case CIV_IMAGE_CAP: {
                        TRYC(rstr_as_int(arg, &civ->defaults.image_cap));
                    } break;
                    default: ABORT("unhandled id: %u", id);
                }
            }
            if(!id && !argnb) THROW("invalid config: " F("'%.*s'", BOLD) " in file '%.*s' at line %zu", RSTR_F(arg), STR_F(*path), linenb);
            //printff(">> id %u, %zu", id, argnb)
        }
        if(argnb < 2) THROW("missing value for: " F("'%.*s'", BOLD) " in file '%.*s' at line %zu", RSTR_F(line), STR_F(*path), linenb);
    }

    return 0;
error:
    return -1;
}

#define ERR_civ_config_path(...) "error while getting config path"
[[nodiscard]] int civ_config_path(Civ *civ, Str *path) {
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
        TRYC(str_fmt(path, "%s", word.we_wordv[0]));
        str_cstr(*path, cpath, FILE_PATH_MAX);
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

[[nodiscard]] int civ_config_defaults(Civ *civ) {
    int err = 0;
    CivConfig *defaults = &civ->defaults;
    defaults->font_path = RSTR("/usr/share/fonts/MonoLisa/ttf/MonoLisa-Regular.ttf");
    defaults->font_size = 18;
    defaults->show_description = false;
    defaults->show_loaded = true;
    defaults->jobs = sysconf(_SC_NPROCESSORS_ONLN);
    defaults->shuffle = false;
    defaults->image_cap = 0;

    /* finally, do defaults here */
    civ->config = *defaults;

    /* load config */
    Str path = {0};
    TRYC(civ_config_path(civ, &path));
    TRYC(civ_config_load(civ, &path));
    //printff("PATH: '%.*s'", STR_F(path));

clean:
    str_free(&path);
    return err;
error:
    ERR_CLEAN;
}



