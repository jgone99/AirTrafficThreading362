#include <pthread.h>

#ifndef _QUEUE_H
#define _QUEUE_H

/* Data types and function prototypes for a queue of names structure */

typedef struct queue {
    pthread_mutex_t lock;
    pthread_cond_t waiter;
    struct node *first;
    struct node *last;
} queue;

void queue_print(queue *q);
void queue_init(queue *q);
void queue_enqueue(queue *q, char *name);
char *queue_front(queue *q);
void queue_dequeue(queue *q);
void queue_destroy(queue *q);

// For cond/wait examples

char *queue_dequeue_wait3(queue *q);

#endif
