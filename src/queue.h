#ifndef QUEUE_H

#include <rlso.h>
#include <rlpw.h>
#include "civ.h"

typedef struct QueueDo {
    Civ *civ;
    So file_or_dir;
    Image img;
} QueueDo;

QueueDo *queue_do(QueueDo *qd, So file_or_dir);
int queue_walk(So file_or_dir, void *void_qd);

//void queue_walk(Pw *pw, So file_or_dir);
//void queue_load(Pw *pw, So file_or_dir);

#define QUEUE_H
#endif

