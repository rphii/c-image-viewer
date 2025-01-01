#include <unistd.h>
#include "civ.h"
#include "stb_image.h"

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
    //if(image->data) printf("LOADED '%s'\n", image->filename);

exit:
    /* finished this thread .. make space for next thread */
    pthread_mutex_lock(&image_load->queue->mutex);
    image_load->queue->q[(image_load->queue->i0 + image_load->queue->len) % image_load->queue->jobs] = image_load;
    ++image_load->queue->len;
    ++(*image_load->queue->done);
    pthread_mutex_unlock(&image_load->queue->mutex);

    return 0;
}


#if 0
typedef enum {
    FILE_TYPE_NONE,
    FILE_TYPE_FILE,
    FILE_TYPE_DIR,
    FILE_TYPE_ERROR,
} FileTypeList;

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

FileTypeList file_get_type(const char *filename) {
#if defined(PLATFORM_WINDOWS)
    ASSERT("not implemented");
#else
    struct stat s;
    int r = lstat(filename, &s);
    if(r) return FILE_TYPE_ERROR;
    if(S_ISREG(s.st_mode)) return FILE_TYPE_FILE;
    if(S_ISDIR(s.st_mode)) return FILE_TYPE_DIR;
#endif
    return 0;
}

#define FILE_PATH_MAX   4096

typedef int (*FileFunc)(const char *filename, void *);
int file_exec(const char *dirname, VStr *subdirs, bool recursive, FileFunc exec, void *args) {
    int err = 0;
    DIR *dir = 0;
    char *subdir = 0;
    //printf("FILENAME: %.*s\n", STR_F(dirname));
    FileTypeList type = file_get_type(dirname);
    if(type == FILE_TYPE_DIR) {
        if(!recursive) {
            fprintf(stderr, "will not go over '%s' (enable recursion to do so)", dirname);
            goto error;
        }
        size_t len = strrchr(dirname, PLATFORM_CH_SUBDIR) - dirname;
        //size_t len = str_rnch(dirname, PLATFORM_CH_SUBDIR, 0);
        if(len < strlen(dirname) && dirname[len] != PLATFORM_CH_SUBDIR) ++len;
        struct dirent *dp = 0;
        //char cdir[FILE_PATH_MAX];
        //str_cstr(dirname, cdir, FILE_PATH_MAX);
        char *cdir = dirname;
        if((dir = opendir(cdir)) == NULL) {
            goto clean;
        }
        char filename[FILE_PATH_MAX] = {0};
        while ((dp = readdir(dir)) != NULL) {
            if(dp->d_name[0] == '.') continue; // TODO add an argument for this
            if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) continue;
            size_t len2 = snprintf(filename, FILE_PATH_MAX, "%.*s/%s", (int)len, cdir, dp->d_name);
            if(len2 != strlen(filename)) assert(0 && "should probably have len2!");
            //--len;
            char filename2[FILE_PATH_MAX];
            snprintf(filename2, FILE_PATH_MAX, "%.*s", (int)len2, filename);
            FileTypeList type2 = file_get_type(filename2);
            if(type2 == FILE_TYPE_DIR) {
                //snprintf(subdir, FILE_PATH_MAX, "%.*s", filename2);
                size_t len2 = strlen(filename2);
                subdir = malloc(len2 + 1);
                if(!subdir) goto error;
                strncpy(subdir, filename2, len2);
                subdir[len2] = 0;
                //TRYC(str_fmt(&subdir, "%.*s", STR_F(filename2)));
                vstr_push_back(subdirs, subdir);
                //str_zero(&subdir);
            } else if(type2 == FILE_TYPE_FILE) {
                exec(filename2, args); //, "an error occured while executing the function");
            } else {
                //info(INFO_skipping_nofile_nodir, "skipping '%.*s' since no regular file nor directory", STR_F(*dirname));
            }
        }
    } else if(type == FILE_TYPE_FILE) {
        exec(dirname, args); //, "an error occured while executing the function");
    } else if(type == FILE_TYPE_ERROR) {
        fprintf(stderr, "failed checking type of '%s' (maybe it doesn't exist?)", dirname);
        goto error;
    } else {
        //info(INFO_skipping_nofile_nodir, "skipping '%.*s' since no regular file nor directory", STR_F(*dirname));
    }
clean:
    //str_free(&subdir);
    if(dir) closedir(dir);
    return err;
error:
    err = -1;
    goto clean;;
}

int image_push_back(const char *filename, void *data) {
    ImageLoadArgs *image_load = data;
    printf("PUSH BACK: '%s'\n", filename);
    Image push = { .filename = filename };
    //pthread_mutex_lock(image_load->mutex);
    vimage_push_back(image_load->images, &push);
    printf("LENGTH: %zu\n", vimage_length(*image_load->images));
    //pthread_mutex_unlock(image_load->mutex);
    return 0;
}
#endif


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

void civ_defaults(Civ *civ) {
    CivConfig *defaults = &civ->defaults;
    defaults->font_path = RSTR("/usr/share/fonts/MonoLisa/ttf/MonoLisa-Regular.ttf");
    defaults->font_size = 18;
    defaults->show_description = false;
    defaults->show_loaded = true;
    defaults->jobs = sysconf(_SC_NPROCESSORS_ONLN);

    /* finally, do defaults here */
    civ->config = *defaults;
}

void civ_arg(Civ *civ, const char *name) {
    Arg *arg = &civ->arg;
    arg_allow_rest(arg, RSTR("images"));
    TRYC(arg_attach_help(arg, RSTR("image viewer written in C"), RSTR("https://github.com/rphii/c-image-viewer")));
    //arg_allow_rest(arg, "images");
    ArgOpt *obj;
    CivConfig *defaults = &civ->defaults;
    CivConfig *config = &civ->config;
    /* font */
    obj = argopt_new(arg, &arg->options, 'f', RSTR("--font"), RSTR("specify font"));
    arg_str(obj, &config->font_path, &defaults->font_path, false, ARG_NO_CALLBACK);
    obj = argopt_new(arg, &arg->options, 'F', RSTR("--font-size"), RSTR("specify font size"));
    arg_int(obj, &config->font_size, &defaults->font_size, false, ARG_NO_CALLBACK, 0, 0);
    obj = argopt_new(arg, &arg->options, 'd', RSTR("--description"), RSTR("toggle description on/off"));
    arg_bool(obj, &config->show_description, &defaults->show_description, false, ARG_NO_CALLBACK);
    obj = argopt_new(arg, &arg->options, '%', RSTR("--loaded"), RSTR("toggle loading info on/off"));
    arg_bool(obj, &config->show_loaded, &defaults->show_loaded, false, ARG_NO_CALLBACK);
    obj = argopt_new(arg, &arg->options, 'j', RSTR("--jobs"), RSTR("set jobs"));
    arg_int(obj, &config->jobs, &defaults->jobs, false, ARG_NO_CALLBACK, 0, 0);
    obj = argopt_new(arg, &arg->options, 's', RSTR("--filter"), RSTR("set filter"));
    ArgOpt *filter = arg_option(obj, (ssize_t *)&civ->filter);
    obj = argopt_new(arg, filter->options, 0, RSTR("nearest"), RSTR("set nearest"));
    arg_enum(obj, false, ARG_NO_CALLBACK, FILTER_NEAREST);
    obj = argopt_new(arg, filter->options, 0, RSTR("linear"), RSTR("set linear"));
    arg_enum(obj, false, ARG_NO_CALLBACK, FILTER_LINEAR);
error:
    /* TODO: fix ugly code */
    return;
}

