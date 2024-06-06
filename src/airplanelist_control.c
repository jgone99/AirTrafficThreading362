#include <string.h>
#include <malloc.h>

#include "alist.h"
#include "airplane.h"
#include "airplanelist_control.h"

alist *planelist;

pthread_rwlock_t lock;

void create_planelist() {
    planelist = malloc(sizeof(alist));
    alist_init(planelist, (void (*)(void *))airplane_destroy);
}
/**
 * @brief If airplane exists, returns index of airplane with corresponding 
 * flight id. Otherwise returns -1
 * 
 * @param flightid 
 * @return int 
 */
int planelist_findbyid(char *flightid) {
    if(strcmp(flightid, "\0") == 0) return -1;
    for(int index = 0; index < planelist->in_use; index++) {
        if(strcmp(((airplane *)alist_get(planelist, index))->id, flightid) == 0) {
            return index;
        }
    }
    return -1;
}

void planelist_add(airplane *plane, char *flightid) {
    if(plane == NULL) return;
    pthread_rwlock_wrlock(&lock);
    strcpy(plane->id, flightid);
    alist_add(planelist, plane);
    pthread_rwlock_unlock(&lock);
    
}

void planelist_remove(airplane *plane) {
    if(plane == NULL) return;
    int index = -1;
    pthread_rwlock_wrlock(&lock);
    if(plane->id == NULL) {
        return;
    }
    if((index=planelist_findbyid(plane->id)) != -1) {
        alist_remove(planelist, index);
    }
    pthread_rwlock_unlock(&lock);
}

/**
 * @brief Returns 1 if flightid already exists in the airplanelist.
 * Returns 0 if flightid does not exist. Returns -1 if flightid id NULL
 * 
 * @param flightid 
 * @return int 
 */
int planelist_containsid(char *flightid) {
    if(flightid == NULL) {
        return -1;
    }
    int index = -1;
    pthread_rwlock_rdlock(&lock);
    index = planelist_findbyid(flightid);
    pthread_rwlock_unlock(&lock);
    return index != -1;
}

void *planelist_find(char *flightid) {
    pthread_rwlock_rdlock(&lock);
    int index = planelist_findbyid(flightid);
    airplane *plane = alist_get(planelist, index);
    pthread_rwlock_unlock(&lock);
    return plane;
}

void destroy_planelist() {
    alist_destroy(planelist);
    free(planelist);
    planelist = NULL;
}