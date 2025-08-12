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
    return q;
}

void queue_done(QueueDo *qd) {
    so_free(&qd->file_or_dir);
    free(qd);
}

void *queue_do_walk(void *void_qd);
void *queue_do_load(void *void_qd);

int queue_walk(So file_or_dir, void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = queue_do(void_qd, file_or_dir);
    //printff("w [%.*s]", SO_F(qd->file_or_dir));
    pw_queue(&qd->civ->pw, queue_do_walk, qd);
    return 0;
}

int queue_load(So file_or_dir, void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = queue_do(void_qd, file_or_dir);
    //printff("l [%.*s]", SO_F(qd->file_or_dir));
    pw_queue(&qd->civ->pw, queue_do_load, qd);
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
void *queue_do_walk(void *void_qd) {
    QueueDo *qd = void_qd;
    (void) so_file_exec(qd->file_or_dir, true, true, queue_load, queue_walk, void_qd);
    queue_done(qd);
    return 0;
}


