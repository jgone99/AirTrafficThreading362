#ifndef _QUEUE_MANAGER_H
#define _QUEUE_MANAGER_H

#include "queue.h"

void create_planequeue();
void destroy_planequeue();
void planequeue_add(char *flightid);
void planequeue_inair();
void *queue_han_thread(void *args);

#endif