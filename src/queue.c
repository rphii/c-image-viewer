#include "queue.h"
#include <stb/stb_image.h>

QueueDo *queue_do(QueueDo *qd, So file_or_dir) {
    ASSERT_ARG(qd);
    QueueDo *q;
    NEW(QueueDo, q);
    ASSERT_ARG(q);
    memcpy(q, qd, sizeof(*q));
    q->file_or_dir = so_clone(file_or_dir);
    //q->ref = 0;
    //pthread_mutex_init(&q->ref_mtx, 0);
    return q;
}

void queue_done(QueueDo *qd) {
    so_free(&qd->file_or_dir);
    free(qd);
}

void *queue_do_walk(Pw *pw, bool *cancel, void *void_qd);
void *queue_do_load(Pw *pw, bool *cancel, void *void_qd);
void *queue_do_add(Pw *pw, bool *cancel, void *void_qd);

int queue_walk(So file_or_dir, void *void_qd) {
    ASSERT_ARG(void_qd);
    if(!so_len(file_or_dir)) return 0;
    QueueDo *qd = queue_do(void_qd, file_or_dir);
    pw_queue(&qd->civ->queues.file_loader, queue_do_walk, qd);
    return 0;
}

int queue_add(So file_or_dir, void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = queue_do(void_qd, file_or_dir);
    pw_queue(&qd->civ->queues.file_loader, queue_do_add, qd);
    return 0;
}


#include <rlc/vec.h>
VEC_IMPLEMENT(VImage, vimage, Image, BY_REF, SORT2, image_cmp, by_index);
VEC_INCLUDE(VImage, vimage, Image, BY_REF, SORT2, by_index);

void *keep_valid_images(Pw *pw, bool *cancel, void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = void_qd;
    size_t cap = qd->civ->config.image_cap;
    pw_when_done_clear(pw);
    pthread_mutex_lock(&qd->civ->images_mtx);
    size_t len = vimage_length(qd->civ->images_discover);
    pthread_mutex_unlock(&qd->civ->images_mtx);
    size_t added = 0;
    for(size_t i = 0; i < len; ++i) {
        Image *img = vimage_get_at(&qd->civ->images_discover, i);
        if(img->data && (!cap || added < cap)) {
            pthread_mutex_lock(&qd->civ->images_mtx);
            vimage_push_back(&qd->civ->images, img);
            pthread_mutex_unlock(&qd->civ->images_mtx);
            ++added;
        } else {
            image_free(img);
        }
    }
    qd->civ->action_map.gl_update = true;
    glfwPostEmptyEvent();
    free(qd);
    return 0;
}

void *remove_too_many(Pw *pw, bool *cancel, void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = void_qd;
    size_t cap = qd->civ->config.image_cap;
    pw_when_done_clear(pw);
    pthread_mutex_lock(&qd->civ->images_mtx);
    size_t len = vimage_length(qd->civ->images);
    size_t selected = qd->civ->view.selected;
    Image current = {0};
    if(selected < len) {
        current = *vimage_get_at(&qd->civ->images, selected);
    }
    vimage_sort_by_index(&qd->civ->images);
    for(size_t i = 0; qd->civ->config.preview_retain && i < len; ++i) {
        Image *check = vimage_get_at(&qd->civ->images, i);
        if(check->index_pre_loading == current.index_pre_loading) {
            qd->civ->view.selected = i;
            break;
        }
    }
    pthread_mutex_unlock(&qd->civ->images_mtx);
    if(cap) {
        for(size_t i = cap; i < len; ++i) {
            pthread_mutex_lock(&qd->civ->images_mtx);
            Image img = {0};
            vimage_pop_back(&qd->civ->images, &img);
            pthread_mutex_unlock(&qd->civ->images_mtx);
            image_free(&img);

        }
    }
    qd->civ->action_map.gl_update = true;
    glfwPostEmptyEvent();
    free(qd);
    return 0;
}

void *queue_do_add(Pw *pw, bool *cancel, void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = void_qd;
    Image img = {};
    pthread_mutex_lock(&qd->civ->images_mtx);
    vimage_push_back(&qd->civ->images_discover, &img);
    qd->img = vimage_get_back(&qd->civ->images_discover);
    qd->img->filename = qd->file_or_dir;
    pthread_mutex_unlock(&qd->civ->images_mtx);
    free(qd);
    return 0;
}

int image_cmp_by_index(Image *a, Image *b) {
    return a->index_pre_loading - b->index_pre_loading;
}

void load_image(QueueDo *qd) {
    Image load = {0};
    FILE *fp = so_file_fp(qd->img->filename, "r");
    QueueState *qs = qd->civ->qstate;
    if(fp) {
        load.data = stbi_load_from_file(fp, &load.width, &load.height, &load.channels, 0);
        pthread_mutex_lock(&qd->civ->images_mtx);
        if(load.data) {
            qd->img->channels = load.channels;
            qd->img->width = load.width;
            qd->img->height = load.height;
            qd->img->data = load.data;
            ++qd->civ->images_loaded;
            if(qd->civ->config.preview_load) {
                vimage_push_back(&qd->civ->images, qd->img);
            }
            //memset(qd->img, 0, sizeof(*qd->img));
            qd->civ->action_map.gl_update = true;
            glfwPostEmptyEvent();
        } else {
            ++qs->number_of_non_images;
        }
        pthread_mutex_unlock(&qd->civ->images_mtx);
        fclose(fp);
    }
}

void *queue_do_load(Pw *pw, bool *cancel, void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = void_qd;
    QueueState *qs = qd->civ->qstate;
    Image *img = qd->img;
    size_t cap = qd->civ->config.image_cap;
    size_t index_pre = img->index_pre_loading;

    bool attempt_load = false;
    if(!cap || index_pre < cap) {
        ///printff("0 [%.*s]", SO_F(img->filename));
        attempt_load = true;
    } else {
        /* we will always have a cap. check if we still require images */
        pthread_mutex_lock(&qd->civ->images_mtx);
        if(qd->civ->images_loaded < cap) attempt_load = true;
        pthread_mutex_unlock(&qd->civ->images_mtx);
    }

    if(attempt_load) {
        load_image(qd);
    } 

    free(qd);
    return 0;
}

void *when_done_gathering(Pw *pw, bool *cancel, void *void_qd) {
    QueueDo *qd = void_qd;
    QueueState *s = qd->civ->qstate;
    pw_when_done_clear(pw);
    size_t len = vimage_length(qd->civ->images_discover);
    //printff("found %zu files...", len);
    if(qd->civ->config.shuffle) {
        //printff("shuffle...");
        for(size_t i = 0; i < len; ++i) {
            size_t i1 = i;
            size_t i2 = rand() % len;
            //printff("  swap %zu <:> %zu", i1, i2);
            vimage_swap(&qd->civ->images_discover, i1, i2);
        }
    } else {
        //printff("sort...");
        vimage_sort(&qd->civ->images_discover);
        //printff("sorted...");
    }

    if(qd->civ->config.preview_load) {
        pw_when_done(pw, remove_too_many, qd);
    } else {
        pw_when_done(pw, keep_valid_images, qd);
    }

    for(size_t i = 0; i < len; ++i) {
        Image *img = vimage_get_at(&qd->civ->images_discover, i);
        img->index_pre_loading = i;
        QueueDo q = {
            .img = img,
            .civ = qd->civ,
        };
        pw_queue(pw, queue_do_load, queue_do(&q, SO));
    }

    return 0;
}

void *queue_do_walk(Pw *pw, bool *cancel, void *void_qd) {
    QueueDo *qd = void_qd;
    (void) so_file_exec(qd->file_or_dir, true, true, queue_add, queue_walk, void_qd);
    queue_done(qd);
    return 0;
}

#include <unistd.h>
void *observe_pipe(Pw *pw, bool *cancel, void *void_data) {
    QueueDo *qd = void_data;
    So input = SO;
    while(true) {
        so_clear(&input);
        so_input(&input);
        if(!so_len(input) && getchar() == EOF) break;
        queue_walk(input, qd);
    }
    free(qd);
    return 0;
}

