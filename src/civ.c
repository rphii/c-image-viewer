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

exit:
    /* finished this thread .. make space for next thread */
    pthread_mutex_lock(&image_load->queue->mutex);
    image_load->queue->q[(image_load->queue->i0 + image_load->queue->len) % PROC_COUNT] = image_load;
    ++image_load->queue->len;
    pthread_mutex_unlock(&image_load->queue->mutex);

    return 0;
}

void images_load(VImage *images, const char **files, long n, pthread_mutex_t *mutex, bool *cancel, long jobs) {

    ImageLoad *image_load = malloc(sizeof(ImageLoad) * jobs);
    if(!image_load) return;
    ImageLoadThreadQueue queue = {0};

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
}

void *images_load_voidptr(void *argp) {
    ImageLoadArgs *args = argp;
    images_load(args->images, args->files, args->n, args->mutex, args->cancel, args->jobs);
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
        case FIT_FILL_XY: return "FILL XY";
        case FIT_PAN: return "PAN";
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

