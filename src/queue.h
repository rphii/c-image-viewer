#ifndef QUEUE_H

#include <rlso.h>
#include <rlpw.h>
#include "civ.h"

typedef struct QueueState {
    //pthread_mutex_t add_mtx;
    //size_t add;
    //pthread_mutex_t add_do_mtx;
    //size_t add_do;
    ///pthread_mutex_t walk_mtx;
    ///size_t walk;
    ///pthread_mutex_t walk_do_mtx;
    ///size_t walk_do;
    //size_t loaded;
    //pthread_mutex_t load_disc_mtx;
    size_t number_of_non_images;
} QueueState;

typedef struct QueueDo {
    Civ *civ;
    So file_or_dir;
    Image *img;
} QueueDo;

QueueDo *queue_do(QueueDo *qd, So file_or_dir);
void *when_done_gathering(Pw *pw, bool *cancel, void *void_qd);

int queue_walk(So file_or_dir, void *void_qd);

void *observe_pipe(Pw *pw, bool *cancel, void *void_data);

//void queue_walk(Pw *pw, So file_or_dir);
//void queue_load(Pw *pw, So file_or_dir);

#define QUEUE_H
#endif

