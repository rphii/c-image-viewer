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

void *queue_do_load(void *void_qd) {
    ASSERT_ARG(void_qd);
    QueueDo *qd = void_qd;
    //printff("l [%.*s]", SO_F(qd->file_or_dir));
    size_t cap = qd->civ->config.image_cap;
    if(!cap || vimage_length(qd->civ->images) < cap) {
        FILE *fp = so_file_fp(qd->file_or_dir, "r");
        if(fp) {
            Image img = {0};
            img.data = stbi_load_from_file(fp, &img.width, &img.height, &img.channels, 0);
            if(img.data) {
                pthread_mutex_lock(&qd->civ->images_mtx);
                if(!cap || vimage_length(qd->civ->images) < cap) {
                    printff("%u ch, %5ux%-5u [%.*s]", img.channels, img.width, img.height, SO_F(qd->file_or_dir));
                    vimage_push_back(&qd->civ->images, &img);
                    *qd->civ->gl_update = true;
                    glfwPostEmptyEvent();
                } else {
                    free(img.data);
                }
                pthread_mutex_unlock(&qd->civ->images_mtx);
            }
            fclose(fp);
        }
    }
    queue_done(qd);
    return 0;
}

void *queue_do_walk(void *void_qd) {
    QueueDo *qd = void_qd;
    (void) so_file_exec(qd->file_or_dir, true, true, queue_load, queue_walk, void_qd);
    queue_done(qd);
    return 0;
}


