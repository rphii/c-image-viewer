#include "queue.h"
#include <stb/stb_image.h>

static pthread_mutex_t mtx;

QueueDo *queue_do(QueueDo *qd, So file_or_dir) {
    ASSERT_ARG(qd);
    QueueDo *q;
    NEW(QueueDo, q);
    ASSERT_ARG(q);
    memcpy(q, qd, sizeof(*q));
    q->file_or_dir = file_or_dir;
    //q->ref = 0;
    //pthread_mutex_init(&q->ref_mtx, 0);
    return q;
}

void queue_done(QueueDo *qd) {
    so_free(&qd->file_or_dir);
    free(qd);
}

void *queue_do_walk(void *void_qd);
void *queue_do_load(void *void_qd);
void *queue_do_add(void *void_qd);

int queue_walk(So file_or_dir, void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = queue_do(void_qd, file_or_dir);
    ///pthread_mutex_lock(&qd->civ->qstate->walk_mtx);
    ///if(!qd->civ->qstate->walk) {
    ///    pw_queue(&qd->civ->pw, queue_watch_filter_launcher, queue_do(void_qd, SO));
    ///}
    ///++qd->civ->qstate->walk;
    ///pthread_mutex_unlock(&qd->civ->qstate->walk_mtx);
    pw_queue(&qd->civ->pw, queue_do_walk, qd);
    return 0;
}

int queue_add(So file_or_dir, void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = queue_do(void_qd, file_or_dir);
    /* reference counting */
    ///pthread_mutex_lock(&qd->civ->qstate->add_mtx);
    ///++qd->civ->qstate->add;
    ///pthread_mutex_unlock(&qd->civ->qstate->add_mtx);
    //printff("l [%.*s]", SO_F(qd->file_or_dir));
    pw_queue(&qd->civ->pw, queue_do_add, qd);
    return 0;
}

void *keep_valid_images(void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = void_qd;
    size_t cap = qd->civ->config.image_cap;
    pw_when_done_clear(&qd->civ->pw);
    size_t len = vimage_length(qd->civ->images_discover);
    size_t added = 0;
    for(size_t i = len; i > 0; --i) {
        size_t i_real = i - 1;
        //pthread_mutex_lock(&qd->civ->images_mtx);
        Image *img = vimage_get_at(&qd->civ->images_discover, i_real);
        if(img->data && (!cap || added < cap)) {
            pthread_mutex_lock(&qd->civ->images_mtx);
            vimage_push_back(&qd->civ->images, img);
            pthread_mutex_unlock(&qd->civ->images_mtx);
            ++added;
            *qd->civ->gl_update = true;
            glfwPostEmptyEvent();
        } else {
            image_free(img);
        }
    }
    free(qd);
    return 0;
}

void *queue_do_add(void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = void_qd;
    Image img = {};
    pthread_mutex_lock(&qd->civ->images_mtx);
    vimage_push_back(&qd->civ->images_discover, &img);
    qd->img = vimage_get_back(&qd->civ->images_discover);
    qd->img->filename = qd->file_or_dir;
    //*qd->civ->gl_update = true;
    //glfwPostEmptyEvent();
    pthread_mutex_unlock(&qd->civ->images_mtx);
    /* reference counting */
    ///pthread_mutex_lock(&qd->civ->qstate->add_do_mtx);
    ///++qd->civ->qstate->add_do;
    ///pthread_mutex_unlock(&qd->civ->qstate->add_do_mtx);
    free(qd);
    return 0;
}

#if 0
void *queue_do_textures_load(void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = void_qd;

    glcontext_acquire(&qd->civ->context, qd->civ->offscreen);
    send_texture_to_gpu(&qd->img, qd->civ->filter, qd->civ->gl_update);
    printff("SENT [%.*s]", SO_F(qd->img.filename));
    /* also make sure the full character set is available */
    So_Uc_Point point = {0};
    for(size_t i = 0; i < so_len(qd->img.filename); ++i) {
        if(so_uc_point(so_i0(qd->img.filename, i), &point)) {
            goto error; /* who cares if there's an invalid utf8 codepoint lol */
        }
        pthread_mutex_lock(&qd->civ->font_mtx);
        font_load_single(qd->civ->font, point.val);
        pthread_mutex_unlock(&qd->civ->font_mtx);
        i += (point.bytes - 1);
    }
    *qd->civ->gl_update = true;
error:
    glcontext_release(&qd->civ->context);
    if(*qd->civ->gl_update) {
        glfwPostEmptyEvent();
    }
    free(qd);
    return 0;
}
#endif

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
            //printff("%zu[%.*s]%ux%ux%u",qd->civ->images_loaded,SO_F(qd->img->filename),load.width,load.height,load.channels);
            //printff("LOADED %.*s TOTAL %zu PTR %p", SO_F(qd->img->filename), qd->civ->images_loaded,qd->img->data);
            //*qd->civ->gl_update = true;
            //glfwPostEmptyEvent();
        } else {
            ++qs->number_of_non_images;
        }
        pthread_mutex_unlock(&qd->civ->images_mtx);
        fclose(fp);
    }
}

#if 1
void *queue_do_load(void *void_qd) {
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
#if 1
        /* we will always have a cap. check if we still require images */
        pthread_mutex_lock(&qd->civ->images_mtx);
        //size_t actual_index = index_pre - qs->number_of_non_images;
        //if(actual_index < cap) attempt_load = true;
        if(qd->civ->images_loaded < cap) attempt_load = true;

        ///if(index_pre > qs->number_of_non_images) {
        ///    if(index_pre - qs->number_of_non_images < cap) {
        ///        //printff("1 [%.*s]", SO_F(img->filename));
        ///        attempt_load = true;
        ///    }
        ///}

        ///if(index_pre <= qs->number_of_non_images && qd->civ->images_loaded < cap) {
        ///    //printff("2 [%.*s]", SO_F(img->filename));
        ///    attempt_load = true;
        ///}

        pthread_mutex_unlock(&qd->civ->images_mtx);
#endif
    }

    if(attempt_load) {
        load_image(qd);
    } 

    free(qd);
    return 0;
}

#else

void *queue_do_load(void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = void_qd;
    //printff("l [%.*s]", SO_F(qd->file_or_dir));
    size_t cap = qd->civ->config.image_cap;
    if(!cap || vimage_length(qd->civ->images) >= cap) goto defer;
    FILE *fp = so_file_fp(qd->file_or_dir, "r");
    if(!fp) goto defer;
    qd->img.data = stbi_load_from_file(fp, &qd->img.width, &qd->img.height, &qd->img.channels, 0);
    if(!qd->img.data) goto defer;
    bool defer = false;
    pthread_mutex_lock(&qd->civ->images_mtx);
    if(!cap || vimage_length(qd->civ->images) < cap) {
        //printff("%u ch, %5ux%-5u [%.*s]", qd->img.channels, qd->img.width, qd->img.height, SO_F(qd->file_or_dir));
        qd->img.filename = qd->file_or_dir;
        vimage_push_back(&qd->civ->images, &qd->img);
        *qd->civ->gl_update = true;
        glfwPostEmptyEvent();
    } else {
        free(qd->img.data);
        defer = true;
    }
    pthread_mutex_unlock(&qd->civ->images_mtx);
    if(defer) goto defer;
    fclose(fp);
#if 0
    pw_queue(&qd->civ->pw, queue_do_textures_load, qd);
#else
    free(qd);
#endif
    return 0;
defer:
    queue_done(qd);
    return 0;
}
#endif

void *queue_watch_filter_launcher(void *void_qd) {
    QueueDo *qd = void_qd;
    QueueState *s = qd->civ->qstate;
    pw_when_done_clear(&qd->civ->pw);
    size_t len = vimage_length(qd->civ->images_discover);
    size_t cap = qd->civ->config.image_cap;
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
#if 0
    if(!cap) {
        printff("no image cap...");
    } else if(len > cap) {
        printff("cap to %zu...", qd->civ->config.image_cap);
        size_t pop = len - cap;
        for(size_t i = 0; i < pop; ++i) {
            vimage_pop_back(&qd->civ->images, 0);
        }
    }
    len = vimage_length(qd->civ->images);
#endif
    pw_when_done(&qd->civ->pw, keep_valid_images, qd);
    for(size_t i = 0; i < len; ++i) {
        Image *img = vimage_get_at(&qd->civ->images_discover, i);
        img->index_pre_loading = i;
        QueueDo q = {
            .img = img,
            .civ = qd->civ,
        };
        pw_queue(&qd->civ->pw, queue_do_load, queue_do(&q, SO));
    }
#if 0
    while(true) {
        bool is_busy = pw_is_busy(&qd->civ->pw);
        if(!is_busy) {
            printff("not busy");
            continue;
        }
        Pw pw = qd->civ->pw;
        printff("busy: %zu, %zu", pw.sched.ready, array_len(pw.queue.data));
        sleep(1);
        //bool wm = pthread_mutex_trylock(&s->walk_mtx);
        //bool wd = pthread_mutex_trylock(&s->walk_do_mtx);
        //bool am = pthread_mutex_trylock(&s->add_mtx);
        //bool ad = pthread_mutex_trylock(&s->add_do_mtx);
        //if(wm || wd || am || ad) {
        //} else {
        //    //printff("add %zu..%zu / %zu..%zu",s->add,s->add_do,s->walk,s->walk_do);
        //    if(s->add == s->add_do && s->walk == s->walk_do) {
        //        printff("found %zu!", s->add_do);
        //        getchar();
        //    }
        //}
        //if(wm) pthread_mutex_unlock(&s->walk_mtx);
        //if(wd) pthread_mutex_unlock(&s->walk_do_mtx);
        //if(am) pthread_mutex_unlock(&s->add_mtx);
        //if(ad) pthread_mutex_unlock(&s->add_do_mtx);
    }
#endif
    return 0;
}

void *queue_do_walk(void *void_qd) {
    QueueDo *qd = void_qd;
    (void) so_file_exec(qd->file_or_dir, true, true, queue_add, queue_walk, void_qd);
    ///pthread_mutex_lock(&qd->civ->qstate->walk_do_mtx);
    ///++qd->civ->qstate->walk_do;
    ///pthread_mutex_unlock(&qd->civ->qstate->walk_do_mtx);
    queue_done(qd);
    ///pthread_mutex_lock(&qd->ref_mtx);
    ///--qd->ref;
    ///pthread_mutex_unlock(&qd->ref_mtx);
    return 0;
}


