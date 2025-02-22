#include <unistd.h>
#include <rphii/file.h>
#include <stb/stb_image.h>

#include "civ.h"

VEC_IMPLEMENT(VImage, vimage, Image, BY_REF, BASE, image_free);
VEC_IMPLEMENT(VImage, vimage, Image, BY_REF, ERR);

#include <rphii/str.h>

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
    Image *image = 0;
    unsigned char *data = 0;
    if(!vimage_length(*image_load->images)) goto exit;

    pthread_mutex_lock(image_load->mutex);
    image = vimage_get_at(image_load->images, image_load->index);
    pthread_mutex_unlock(image_load->mutex);

    if(image->data) goto exit;

    char cfilename[PATH_MAX];
    str_cstr(image->filename, cfilename, PATH_MAX);

    data = stbi_load(cfilename, &image->width, &image->height, &image->channels, 0);
    //printf(">>> STBI_LOAD '%.*s' %zu / %zu\n", STR_F(image->filename), *image_load->queue->done, image_load->image_cap);

exit:
    /* finished this thread's work */
    pthread_mutex_lock(&image_load->queue->mutex);
    /* check to only add as many images as we want */
    if(data) {
        if(!image_load->image_cap || (image_load->image_cap && (ssize_t)image_load->index < image_load->image_cap + (ssize_t)*image_load->queue->failed)) {
            //printf(F(">>> LOADED '%.*s' %zu / %zu\n", FG_GN), STR_F(image->filename), *image_load->queue->done, image_load->image_cap);
            image->data = data;
            ++(*image_load->queue->done);
        } else {
            stbi_image_free(data);
        }
    } else {
        printf(">>> Load failed '%.*s'\n", STR_F(image->filename));
        ++(*image_load->queue->failed);
    }
    /* make space for next thread */
    image_load->queue->q[(image_load->queue->i0 + image_load->queue->len) % image_load->queue->jobs] = image_load;
    ++image_load->queue->len;
    pthread_mutex_unlock(&image_load->queue->mutex);

    glfwPostEmptyEvent();

    return 0;
}

int image_add_to_queue(RStr filename, void *data) {
    VImage *images = data;
    //printff("ADD TO QUEUE: '%.*s'", RSTR_F(filename));
    Image push = {0};
    TRYG(rstr_copy(&push.filename, &filename));
    TRYG(vimage_push_back(images, &push));
    return 0;
error:
    return -1;
}

void images_load(VImage *images, VrStr *files, pthread_mutex_t *mutex, bool *cancel, long jobs, size_t *done, CivConfig *config) {
    ASSERT_ARG(images);
    ASSERT_ARG(files);
    ASSERT_ARG(mutex);
    ASSERT_ARG(cancel);
    ASSERT_ARG(done);
    ASSERT_ARG(config);

    ImageLoad *image_load = malloc(sizeof(ImageLoad) * jobs);
    if(!image_load) return;
    ImageLoadThreadQueue queue = {0};
    size_t failed = 0;
    queue.q = malloc(sizeof(ImageLoad *) * jobs);
    queue.jobs = jobs;
    queue.done = done;
    queue.failed = &failed;
    if(!queue.q) ABORT("job count cannot be 0!");
    VStr subdirs = {0};
    Str subdirname = {0};
    bool recursive = true;

    /* push images to load */
    pthread_mutex_lock(mutex);
#if 1
    size_t n = vrstr_length(*files);
    for(size_t i = 0; i < n; ++i) {
        RStr *filename = vrstr_get_at(files, i);
        if(!filename) ABORT(ERR_UNREACHABLE);
        //vstr_push_back(&subdirs, &STR_LL(rstr_iter_begin(*filename), rstr_length(*filename)));
        /* now iterate over all subdirs until there are none */
        TRYC(file_exec(*filename, &subdirs, recursive, image_add_to_queue, images));
        while(vstr_length(subdirs)) {
            vstr_pop_back(&subdirs, &subdirname);
            //memset(subdirs.items[vstr_length(subdirs)], 0, sizeof(Str)); // TODO: this should probably happen in my vector!
            TRYC(file_exec(str_rstr(subdirname), &subdirs, recursive, image_add_to_queue, images));
            //str_free(&subdirname);
        }
    }
    vstr_free(&subdirs);

    /* shuffle if requested */
    if(config->shuffle) {
        size_t n = vimage_length(*images);
        for(size_t i = 0; i < n; ++i) {
            size_t i1 = i;
            size_t i2 = rand() % n;
            //printff("SWAP %zu - %zu\n", i1, i2);
            vimage_swap(images, i1, i2);
        }
    }

#if 0
    /* remove images if requrested */
    if(config->image_cap) {
        images->last = images->first + config->image_cap;
    }
#endif
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
        image_load[i].image_cap = config->image_cap;
        /* add to queue */
        queue.q[i] = &image_load[i];
        ++queue.len;
    }

    assert(queue.len <= jobs);

    /* load images */
    for(size_t i = 0; i < vimage_length(*images) && !*cancel; ++i) {
        /* this section is responsible for starting the thread */
        if(!(!config->image_cap || (config->image_cap && (ssize_t)i < config->image_cap + (ssize_t)failed))) break;
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
    free(image_load);
    free(queue.q);

    /* remove invalid images */
    pthread_mutex_lock(mutex);
    for(size_t i = 0; i < vimage_length(*images) && !*cancel; ++i) {
        Image *image = vimage_get_at(images, i);
        if(!image->data) {
            vimage_pop_at(images, i--, 0);
        }
    }
    printf(">>> Load finished\n");
    pthread_mutex_unlock(mutex);
    glfwPostEmptyEvent();

    return;

error: ABORT(ERR_UNREACHABLE);
}

void *images_load_voidptr(void *argp) {
    ImageLoadArgs *args = argp;
    images_load(args->images, args->files, args->mutex, args->cancel, args->jobs, &args->done, args->config);
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
    /* TODO: this SHOULD be freed, I THINK, but if I do that I get double free ... AHHHHHHHHH */
    //str_free(&image->filename);
    memset(image, 0, sizeof(*image));
}


const char *fit_cstr(FitList id) {
    switch(id) {
        case FIT_XY: return "XY";
        case FIT_FILL: return "FILL";
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
    pthread_join(state->loader.thread, 0);
    pthread_mutex_lock(state->loader.mutex);
    vimage_free(&state->images);
    pthread_mutex_unlock(state->loader.mutex);
    str_free(&state->config_content);
    arg_free(&state->arg);
    /* done freeing; set zero */
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

void civ_cmd_print_stdout(Civ *civ, bool print_stdout) {
    if(!print_stdout) return;
    if(!civ->active) return;
    pthread_mutex_lock(civ->loader.mutex);
    printf("%.*s\n", STR_F(civ->active->filename));
    pthread_mutex_unlock(civ->loader.mutex);
    civ_popup_set(civ, POPUP_PRINT_STDOUT);
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

    arg_bool(argopt_new(arg, &arg->options, 'S', RSTR("--shuffle"), RSTR("shuffle images before loading")), &config->shuffle, &defaults->shuffle, false, ARG_NO_CALLBACK);
    arg_int(argopt_new(arg, &arg->options, 'C', RSTR("--image-cap"), RSTR("limit number of images to be loaded, 0 to load all")), &config->image_cap, &defaults->image_cap, false, ARG_NO_CALLBACK, 0, 0);

error:
    /* TODO: fix ugly code */
    return;
}

