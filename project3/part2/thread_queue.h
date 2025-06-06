#ifndef THREAD_QUEUE_H
#define THREAD_QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

typedef struct qnode{
    int attendant_id;
    struct qnode *next;
} qnode_t;

typedef struct{
    qnode_t *head;
    qnode_t *tail;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    int size;
    int closed;
} thread_queue_t;

void tq_init(thread_queue_t *q);

void tq_destroy(thread_queue_t *q);

void tq_enqueue(thread_queue_t *q, int attendant_id);

int tq_dequeue(thread_queue_t *q);

int tq_timed_dequeue(thread_queue_t *q, int *out_attendant_id, const struct timespec *abstime);

void tq_close(thread_queue_t *q);

#endif