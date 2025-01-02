#include <unistd.h>
#include "civ.h"
#include "stb_image.h"
#include "file.h"

VEC_IMPLEMENT(VImage, vimage, Image, BY_REF, BASE, image_free);

#include "str.h"

void send_texture_to_gpu(Image *image, FilterList filter, bool *render) {
    if(!image) return;
    if(!image->data) return;

    if(filter == FILTER_NONE) return;
    if(filter >= FILTER__COUNT) return;
    if(image->sent == filter) return;

    if(!image->sent && image->data) {
#if 1
        glGenTextures(1, &image->texture);
        GLenum format;
        if(image->channels == 1) format = GL_RED;
        else if(image->channels == 2) format = GL_RG;
        else if(image->channels == 3) format = GL_RGB;
        else if(image->channels == 4) format = GL_RGBA;
        else {
            fprintf(stderr, "[TEXTURE] Error: number of channels %u\n", image->channels);
            assert(0 && "unsupported image channels");
        }
#endif
#if 1
        glBindTexture(GL_TEXTURE_2D, image->texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, image->width, image->height, 0, format, GL_UNSIGNED_BYTE, image->data);
#else
        //glGenBuffers(1, &image->texture);
        //glGenTextures(1, &image->texture);
        glGenBuffers(1, &image->texture);
        //glBindTexture(GL_TEXTURE_2D, image->texture);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, image->texture);
        void *p = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        ASSERT_ARG(p);
        memcpy(p, image->data, image->width * image->height * image->channels);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
#endif

        stbi_image_free(image->data);
        image->freed_from_cpu = true;
        //image->data = 0;
    }
    if(image->sent && image->data) {
        glBindTexture(GL_TEXTURE_2D, image->texture);
    }

    switch(filter) {
        case FILTER_LINEAR: {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } break;
        case FILTER_NEAREST: {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } break;
        default: break;
    }
    if(image->data) {
        image->sent = filter;
        if(render) *render = true;
    }
}

#include <limits.h>

void *image_load_thread(void *args) {
    ImageLoad *image_load = args;
    if(!vimage_length(*image_load->images)) goto exit;

    pthread_mutex_lock(image_load->mutex);
    Image *image = vimage_get_at(image_load->images, image_load->index);
    pthread_mutex_unlock(image_load->mutex);

    if(image->data) goto exit;

    char cfilename[PATH_MAX];
    rstr_cstr(image->filename, cfilename, PATH_MAX);

    image->data = stbi_load(cfilename, &image->width, &image->height, &image->channels, 0);

    //pthread_mutex_lock(image_load->mutex);
    //pthread_mutex_unlock(image_load->mutex);
    //send_texture_to_gpu(image, FILTER_NEAREST, 0);
    glfwPostEmptyEvent();
    //if(image->data) printf("LOADED '%.*s'\n", RSTR_F(image->filename));

exit:
    /* finished this thread .. make space for next thread */
    pthread_mutex_lock(&image_load->queue->mutex);
    image_load->queue->q[(image_load->queue->i0 + image_load->queue->len) % image_load->queue->jobs] = image_load;
    ++image_load->queue->len;
    ++(*image_load->queue->done);
    pthread_mutex_unlock(&image_load->queue->mutex);

    return 0;
}

void images_load(VImage *images, VrStr *files, pthread_mutex_t *mutex, bool *cancel, long jobs, size_t *done) {
    ASSERT_ARG(images);
    ASSERT_ARG(files);
    ASSERT_ARG(mutex);
    ASSERT_ARG(cancel);
    ASSERT_ARG(done);

    ImageLoad *image_load = malloc(sizeof(ImageLoad) * jobs);
    if(!image_load) return;
    ImageLoadThreadQueue queue = {0};
    queue.q = malloc(sizeof(ImageLoad *) * jobs);
    queue.jobs = jobs;
    queue.done = done;
    assert(queue.q);

    /* push images to load */
    pthread_mutex_lock(mutex);
    size_t n = vrstr_length(*files);
#if 0
    VStr subdirs = {0};
    for(long i = 0; i < n; ++i) {
        const char *filename = vrstr_get_at(files, i);
        ImageLoadArgs args = {
            .images = images,
            .mutex = mutex,
        };
        file_exec(filename, &subdirs, true, image_push_back, &args);
#if 0
        FileTypeList type = file_get_type(filename);
        if(type == FILE_TYPE_FILE) {
            printf("%u file : '%s'\n", type, filename);
        } else if(type == FILE_TYPE_DIR) {
            printf("%u dir : '%s'\n", type, filename);
        }
#endif
    }
#else
    for(size_t i = 0; i < n; ++i) {
        RStr *filename = vrstr_get_at(files, i);
        Image push = { .filename = *filename };
        vimage_push_back(images, &push);
    }
#endif
    pthread_mutex_unlock(mutex);

    pthread_mutex_init(&queue.mutex, 0);
    pthread_attr_init(&queue.attr);
    pthread_attr_setdetachstate(&queue.attr, PTHREAD_CREATE_DETACHED);

    for(int i = 0; i < jobs; ++i) {
        image_load[i].mutex = mutex;
        image_load[i].queue = &queue;
        image_load[i].images = images;
        /* add to queue */
        queue.q[i] = &image_load[i];
        ++queue.len;
    }

    assert(queue.len <= jobs);

    /* load images */
    for(size_t i = 0; i < n && !*cancel; ++i) {
        /* this section is responsible for starting the thread */
        pthread_mutex_lock(&queue.mutex);
        if(queue.len) {
            /* load job */
            ImageLoad *job = queue.q[queue.i0];
            job->index = i;
            /* create thread */
            pthread_create(&job->queue->id, &queue.attr, image_load_thread, job);
            queue.i0 = (queue.i0 + 1) % jobs;
            --queue.len;
        } else {
            --i;
        }
        pthread_mutex_unlock(&queue.mutex);
    }

    /* wait until all threads finished */
    for(;;) {
        pthread_mutex_lock(&queue.mutex);
        if(queue.len == jobs) {
            pthread_mutex_unlock(&queue.mutex);
            break;
        }
        pthread_mutex_unlock(&queue.mutex);
    }
    /* now free since we *know* all threads finished, we can ignore usage of the lock */
    for(long i = 0; i < jobs; ++i) {
        if(image_load[i].queue->id) {
            //str_free(&thr_search[i].content);
            //str_free(&thr_search[i].cmd);
        }
    }
    free(image_load);
    free(queue.q);

    pthread_mutex_lock(mutex);
    for(size_t i = 0; i < vimage_length(*images); ++i) {
        Image *image = vimage_get_at(images, i);
        if(!image->data) {
            printf(">>> removed '%.*s'\n", RSTR_F(image->filename));
            vimage_pop_at(images, i--, 0);
        }
    }
    pthread_mutex_unlock(mutex);
    glfwPostEmptyEvent();
}

void *images_load_voidptr(void *argp) {
    ImageLoadArgs *args = argp;
    images_load(args->images, args->files, args->mutex, args->cancel, args->jobs, &args->done);
    return 0;
}

void images_load_async(ImageLoadArgs *args) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&args->thread, &attr, images_load_voidptr, args);
}

void image_free(Image *image) {
    if(image->data) {
        glDeleteTextures(1, &image->texture);
        if(!image->freed_from_cpu) {
            stbi_image_free(image->data);
        }
    }
    memset(image, 0, sizeof(*image));
}


const char *fit_cstr(FitList id) {
    switch(id) {
        case FIT_XY: return "XY";
        case FIT_X: return "X";
        case FIT_Y: return "Y";
        case FIT_FILL_XY: return "XY-FILL";
        case FIT_PAN: return "PAN";
        default: return "";
    }
}

const char *filter_cstr(FilterList id) {
    switch(id) {
        case FILTER_LINEAR: return "linear";
        case FILTER_NEAREST: return "nearest";
        default: return "";
    }
}

void civ_free(Civ *state) {
    pthread_mutex_lock(state->loader.mutex);
    vimage_free(&state->images);
    pthread_mutex_unlock(state->loader.mutex);
    pthread_join(state->loader.thread, 0);
    memset(state, 0, sizeof(*state));
    str_free(&state->config_content);
}

void civ_popup_set(Civ *state, PopupList id) {
    if(id >= POPUP__COUNT) return;
    state->popup.active = id;
    if(id) {
        timer_start(&state->popup.timer, CLOCK_REALTIME, 0.33);
    }
}

/* commands {{{ */

void civ_cmd_random(Civ *civ, bool random) {
    if(!random) return;
    size_t n_img = vimage_length(civ->images);
    if(!n_img) return;
    civ_cmd_select(civ, rand() % n_img);
}

void civ_cmd_select(Civ *civ, int change) {
    if(!change) return;
    glm_vec2_zero(civ->pan);
    civ->zoom.initial = 0.0f;
    civ->zoom.current = 1.0f;
    size_t n_img = vimage_length(civ->images);
    /* cap index, in case size changd */
    if(civ->selected > n_img) civ->selected = n_img ? n_img - 1 : 0;
    /* adjust */
    if(n_img) {
        if(change < 0) {
            size_t sel = (-change) % n_img;
            civ->selected += n_img - sel;
        } else {
            civ->selected += change;
        }
        civ->selected %= n_img;
        civ->fit.current = civ->fit.initial;
    }
    civ_popup_set(civ, POPUP_SELECT);
}

void civ_cmd_fit(Civ *civ, bool next) {
    if(!next) return;
    /* if we're zoomed in, don't go next, but re-set the current to initial */
    if(!civ->zoom.initial) {
        ++civ->fit.initial;
        civ->fit.initial %= FIT__COUNT;
    }
    civ->fit.current = civ->fit.initial;
    civ->zoom.initial = 0.0f;
    civ->zoom.current = 1.0f;
    glm_vec2_zero(civ->pan);
    civ_popup_set(civ, POPUP_FIT);
}

void civ_cmd_description(Civ *civ, bool toggle) {
    if(!toggle) return;
    civ->config.show_description = !civ->config.show_description;
    civ_popup_set(civ, POPUP_DESCRIPTION);
}

void civ_cmd_zoom(Civ *civ, double zoom) {
    if(!zoom) return;
    if(!civ->active) return;
    /* TODO: make zoom dependant on image-to-window ratio ... */
    civ->zoom.initial = civ->zoom.current;
    if(zoom < 0) {
        civ->zoom.current *= (1.0 + zoom);
    } else if(zoom > 0) {
        civ->zoom.current /= (1.0 - zoom);
    }
    //printf("ZOOM : %f\n", civ->zoom);
    civ->fit.current = FIT_PAN;
    civ_popup_set(civ, POPUP_ZOOM);
}

void civ_cmd_filter(Civ *civ, bool next) {
    if(!next) return;
    ++civ->filter;
    civ->filter %= FILTER__COUNT;
    if(civ->filter == 0) ++civ->filter;
    civ_popup_set(civ, POPUP_FILTER);
}

void civ_cmd_pan(Civ *civ, vec2 pan) {
    if(!pan[0] && !pan[1]) return;
    civ->zoom.initial = civ->zoom.current;
    civ->pan[0] += pan[0] / civ->zoom.current; //s_action.pan_x / s_civ.wwidth * civ->zoom;
    civ->pan[1] -= pan[1] / civ->zoom.current; //s_action.pan_y / s_civ.wheight * civ->zoom;
    civ->fit.current = FIT_PAN;
    civ_popup_set(civ, POPUP_PAN);
}

/* commands }}} */

#include <wordexp.h>

#define ERR_civ_config_path(...) "error while getting config path"
[[nodiscard]] int civ_config_path(Civ *civ, Str *path) {
    ASSERT_ARG(civ);
    ASSERT_ARG(path);
    const char *paths[] = {
        "$XDG_CONFIG_HOME/imv/config",
        "$HOME/.config/civ/config",
        "/etc/civ/config"
    };
    char cpath[PATH_MAX];
    for(size_t i = 0; i < SIZE_ARRAY(paths); ++i, str_clear(path)) {
        wordexp_t word;
        if(wordexp(paths[i], &word, 0)) {
            continue;
        }
        if(!word.we_wordv[0]) {
            wordfree(&word);
            continue;
        }
        TRYC(str_fmt(path, "%s", word.we_wordv[0]));
        str_cstr(*path, cpath, PATH_MAX);
        if(!strlen(cpath) || access(cpath, R_OK) == -1) {
            continue;
        }
        break;
    }
    return 0;
error:
    return -1;
}

RStr civ_config_list(CivConfigList id) {
    switch(id) {
        case CIV_FONT_PATH: return RSTR("font-path");
        case CIV_FONT_SIZE: return RSTR("font-size");
        case CIV_SHOW_DESCRIPTION: return RSTR("show-description");
        case CIV_SHOW_LOADED: return RSTR("show-loaded");
        case CIV_JOBS: return RSTR("jobs");
        case CIV_QAFL: return RSTR("quit-after-full-load");
        default: ABORT("unsupported id: '%u'", id);
    }
}

#define ERR_civ_config_load(...) "error while loading config"
[[nodiscard]] int civ_config_load(Civ *civ, Str *path) {
    ASSERT_ARG(civ);
    ASSERT_ARG(path);
    size_t linenb = 1;
    Str *content = &civ->config_content;
    TRYC(file_str_read(path, content));
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
                    if(rstr_cmp(&cmp, &arg)) continue;
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

[[nodiscard]] int civ_defaults(Civ *civ) {
    int err = 0;
    CivConfig *defaults = &civ->defaults;
    defaults->font_path = RSTR("/usr/share/fonts/MonoLisa/ttf/MonoLisa-Regular.ttf");
    defaults->font_size = 18;
    defaults->show_description = false;
    defaults->show_loaded = true;
    defaults->jobs = sysconf(_SC_NPROCESSORS_ONLN);

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

void civ_arg(Civ *civ, const char *name) {
    Arg *arg = &civ->arg;
    arg_allow_rest(arg, RSTR("images"));
    TRYC(arg_attach_help(arg, RSTR("image viewer written in C"), RSTR("https://github.com/rphii/c-image-viewer")));
    //arg_allow_rest(arg, "images");
    CivConfig *defaults = &civ->defaults;
    CivConfig *config = &civ->config;
    /* font */
    arg_str(argopt_new(arg, &arg->options, 'f', RSTR("--font"), RSTR("specify font")), &config->font_path, &defaults->font_path, false, ARG_NO_CALLBACK);
    arg_int(argopt_new(arg, &arg->options, 'F', RSTR("--font-size"), RSTR("specify font size")), &config->font_size, &defaults->font_size, false, ARG_NO_CALLBACK, 0, 0);
    arg_bool(argopt_new(arg, &arg->options, 'd', RSTR("--description"), RSTR("toggle description on/off")), &config->show_description, &defaults->show_description, false, ARG_NO_CALLBACK);
    arg_bool(argopt_new(arg, &arg->options, '%', RSTR("--loaded"), RSTR("toggle loading info on/off")), &config->show_loaded, &defaults->show_loaded, false, ARG_NO_CALLBACK);
    arg_bool(argopt_new(arg, &arg->options, 0, RSTR("--quit-after-full-load"), RSTR("quit after fully loading")), &config->qafl, &defaults->qafl, false, ARG_NO_CALLBACK);
    arg_int(argopt_new(arg, &arg->options, 'j', RSTR("--jobs"), RSTR("set maximum jobs to use when loading")), &config->jobs, &defaults->jobs, false, ARG_NO_CALLBACK, 0, 0);

    ArgOpt *filter = arg_option(argopt_new(arg, &arg->options, 's', RSTR("--filter"), RSTR("set filter")), (ssize_t *)&civ->filter);
    arg_enum(argopt_new(arg, filter->options, 0, RSTR("nearest"), RSTR("set nearest")), false, ARG_NO_CALLBACK, FILTER_NEAREST);
    arg_enum(argopt_new(arg, filter->options, 0, RSTR("linear"), RSTR("set linear")), false, ARG_NO_CALLBACK, FILTER_LINEAR);

error:
    /* TODO: fix ugly code */
    return;
}

