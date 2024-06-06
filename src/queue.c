#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/*
 * This is a "queue of strings" implementation. The queue is implemented
 * using a linked list, where new items are added at the end of the list,
 * and items are removed from the front of the list.
 *
 * Values on the queue are strings, and a copy is made before storing in
 * the queue. Users of this queue should always call queue_init() to
 * initialize a new queue, and should always call queue_destroy() when
 * finished (to free the space).
 */

struct node {
    char *name;
    struct node *next;
};

/******************************************************************
 * Initialize a queue (it starts empty)
 */
void queue_init(queue *q) {
    q->first = q->last = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->waiter, NULL);
}

/******************************************************************
 * Add a new name to the queue
 */
void queue_enqueue(queue *q, char *name) {
    pthread_mutex_lock(&q->lock);
    struct node *newnode = malloc(sizeof(struct node));
    if (newnode == NULL) {
        fprintf(stderr, "Out of memory! Exiting program.\n");
        exit(1);
    }

    newnode->name = malloc(strlen(name)+1);
    strcpy(newnode->name, name);
    newnode->next = NULL;

    if (q->first == NULL)
        q->first = newnode;
    else
        q->last->next = newnode;

    q->last = newnode;
    pthread_cond_signal(&q->waiter);
    pthread_mutex_unlock(&q->lock);
}

/******************************************************************
 * Return the name at the front of the queue. Returns NULL if the
 * queue is empty.
 */
char *queue_front(queue *q) {
    pthread_mutex_lock(&q->lock);
    if (q->first == NULL) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    } else {
        char *name = q->first->name;
        pthread_mutex_unlock(&q->lock);
        return name;
    }
}

/******************************************************************
 * Remove the name at the front of the queue. Calls with an empty
 * queue will be ignored.
 */
void queue_dequeue(queue *q) {
    pthread_mutex_lock(&q->lock);
    if (q->first != NULL) {
        struct node *curr = q->first;
        q->first = q->first->next;
        if (q->first == NULL)
            q->last = NULL;
        free(curr->name);
        free(curr);
    }
    pthread_mutex_unlock(&q->lock);
}

/******************************************************************
 * Removes the front item from the queue, ...
 *
 * Version 3: Threadsafe with locking and a condition variable
 */
char *queue_dequeue_wait3(queue *q) {
    pthread_mutex_lock(&q->lock);
    while (q->first == NULL) {
        pthread_cond_wait(&q->waiter, &q->lock);
    }
    struct node *front = q->first;
    q->first = q->first->next;
    pthread_mutex_unlock(&q->lock);

    char *retval = front->name;
    free(front);
    return retval;
}



/******************************************************************
 * Destroy a queue - frees up all resources associated with the queue.
 */
void queue_destroy(queue *q) {
    pthread_mutex_lock(&q->lock);
    while (q->first != NULL) {
        struct node *curr = q->first;
        q->first = curr->next;
        free(curr->name);
        free(curr);
    }
    pthread_mutex_unlock(&q->lock);
}
