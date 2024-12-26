#include "civ.h"
#include "stb_image.h"

VEC_IMPLEMENT(VImage, vimage, Image, BY_REF, BASE, image_free);

void send_texture_to_gpu(Image *image, FilterList filter, bool *render) {
    if(!image) return;
    if(!image->data) return;

    if(filter == FILTER_NONE) return;
    if(filter >= FILTER__COUNT) return;
    if(image->sent == filter) return;

    if(!image->sent && image->data) {
        GLenum format;
        if(image->channels == 1) format = GL_RED;
        else if(image->channels == 2) format = GL_RG;
        else if(image->channels == 3) format = GL_RGB;
        else if(image->channels == 4) format = GL_RGBA;
        else {
            fprintf(stderr, "[TEXTURE] Error: number of channels %u\n", image->channels);
            assert(0 && "unsupported image channels");
        }

        glGenTextures(1, &image->texture);
        glBindTexture(GL_TEXTURE_2D, image->texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, image->width, image->height, 0, format, GL_UNSIGNED_BYTE, image->data);
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
        *render = true;
    }
}

void *image_load_thread(void *args) {
    ImageLoad *image_load = args;
    if(!vimage_length(*image_load->images)) goto exit;

    pthread_mutex_lock(image_load->mutex);
    Image *image = vimage_get_at(image_load->images, image_load->index);
    pthread_mutex_unlock(image_load->mutex);

    if(image->data) goto exit;

    image->data = stbi_load(image->filename, &image->width, &image->height, &image->channels, 0);
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

void images_load(VImage *images, const char **files, long n, pthread_mutex_t *mutex, bool *cancel, long jobs, long *done) {

    ImageLoad *image_load = malloc(sizeof(ImageLoad) * jobs);
    if(!image_load) return;
    ImageLoadThreadQueue queue = {0};
    queue.q = malloc(sizeof(ImageLoad *) * jobs);
    queue.jobs = jobs;
    queue.done = done;
    assert(queue.q);

    /* push images to load */
    pthread_mutex_lock(mutex);
    for(long i = 0; i < n; ++i) {
        Image push = { .filename = files[i] };
        vimage_push_back(images, &push);
    }
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
    for(long i = 0; i < n && !*cancel; ++i) {
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
}

void *images_load_voidptr(void *argp) {
    ImageLoadArgs *args = argp;
    images_load(args->images, args->files, args->n, args->mutex, args->cancel, args->jobs, &args->done);
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
        stbi_image_free(image->data);
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

void civ_cmd_select(Civ *civ, int change) {
    if(!change) return;
    glm_vec2_zero(civ->pan);
    civ->zoom = 1.0f;
    size_t n_img = vimage_length(civ->images);
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
    // TODO: if we're zoomed in, don't go next, but re-set the current to initial
    ++civ->fit.initial;
    civ->fit.initial %= FIT__COUNT;
    civ->fit.current = civ->fit.initial;
    civ->zoom = 1.0;
    glm_vec2_zero(civ->pan);
    civ_popup_set(civ, POPUP_FIT);
}

void civ_cmd_description(Civ *civ, bool toggle) {
    if(!toggle) return;
    civ->show_description = !civ->show_description;
    civ_popup_set(civ, POPUP_DESCRIPTION);
}

void civ_cmd_zoom(Civ *civ, double zoom) {
    if(!zoom) return;
    if(zoom < 0) {
        civ->zoom *= (1.0 + zoom);
    } else if(zoom > 0) {
        civ->zoom /= (1.0 - zoom);
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
    civ->pan[0] += pan[0] / civ->zoom; //s_action.pan_x / s_civ.wwidth * civ->zoom;
    civ->pan[1] -= pan[1] / civ->zoom; //s_action.pan_y / s_civ.wheight * civ->zoom;
    civ->fit.current = FIT_PAN;
    civ_popup_set(civ, POPUP_PAN);
}

/* commands }}} */

