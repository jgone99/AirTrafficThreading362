
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>

#include "queue_manager.h"
#include "airplane.h"
#include "airplanelist_control.h"

queue *q;
pthread_mutex_t lock;
pthread_cond_t inair_confirm;

void create_planequeue() {
    q = malloc(sizeof(queue));
    queue_init(q);
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&inair_confirm, NULL);
}

void destroy_planequeue() {
    queue_destroy(q);
    free(q);
    q = NULL;
}

void planequeue_add(char *flightid) {
    queue_enqueue(q, flightid);
}

void planequeue_inair() {
    pthread_cond_signal(&inair_confirm);
}

void *queue_han_thread(void *args) {
    create_planequeue();
    airplane *plane = NULL;
    while (1) {
        char * plane_name = queue_dequeue_wait3(q);
        plane = planelist_find(plane_name);
        plane->state = PLANE_CLEAR;
        fprintf(stdout, "Clearing flight %s\n", plane->id);
        fprintf(plane->fp_send, "TAKEOFF\n");
        printf("waiting for inair confirmation\n");
        while(plane->state != PLANE_INAIR) {
            pthread_cond_wait(&inair_confirm, &lock);
        }
        
        printf("inair confirmed\n");
        plane->state = PLANE_DONE;
        sleep(4);
        printf("waited\n");
    }

    destroy_planequeue(q);

    return NULL;
}
