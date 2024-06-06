#ifndef _AIRPLANELIST_CONTROL_H
#define _AIRPLANELIST_CONTROL_H

void create_planelist();
void planelist_add(airplane *plane, char *flightid);
void *planelist_find(char *flightid);
void planelist_remove(airplane *plane);
int planelist_containsid(char *flightid);
void destroy_planelist();

#endif