#include "thread_queue.h"

void tq_init(thread_queue_t *q){
    q->head = q->tail = NULL;
    q->size = 0;
    q->closed = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

void tq_destroy(thread_queue_t *q){
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
}

void tq_close(thread_queue_t *q){
    pthread_mutex_lock(&q->lock);
    q->closed = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

void tq_enqueue(thread_queue_t *q, int attendant_id){
    pthread_mutex_lock(&q->lock);

    if(q->closed){
        pthread_mutex_unlock(&q->lock);
        return;
    }

    qnode_t *node = malloc(sizeof(qnode_t));
    node->attendant_id = attendant_id;
    node->next = NULL;
    if(q->tail == NULL){
        q->head = q->tail = node;
    }
    else{
        q->tail->next = node;
        q->tail = node;
    }
    q->size++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

int tq_dequeue(thread_queue_t *q){
    pthread_mutex_lock(&q->lock);

    while(q->size == 0 && q->closed == 0){
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    if(q->size == 0 && q->closed){
        pthread_mutex_unlock(&q->lock);
        return -1;
    }

    qnode_t *node = q->head;
    int pid = node->attendant_id;
    q->head = node->next;
    if(q->head == NULL) q->tail = NULL;
    q->size--;
    free(node);
    pthread_mutex_unlock(&q->lock);
    return pid;
}

int tq_timed_dequeue(thread_queue_t *q, int *out_attendant_id, const struct timespec *abstime){
    pthread_mutex_lock(&q->lock);

    while(q->size == 0 && q->closed == 0){
        int rc = pthread_cond_timedwait(&q->not_empty, &q->lock, abstime);
        if(rc == ETIMEDOUT){
            pthread_mutex_unlock(&q->lock);
            return -1;
        }
    }

    if(q->size == 0 && q->closed){
        pthread_mutex_unlock(&q->lock);
        return -1;
    }

    qnode_t *node = q->head;
    *out_attendant_id = node->attendant_id;
    q->head = node->next;
    if(q->head == NULL) q->tail = NULL;
    q->size--;
    free(node);
    pthread_mutex_unlock(&q->lock);
    return 0;
}