#ifndef EVENT_H
#define EVENT_H
#include "actions.h"

/*********************************************************************/
/* Event structure. In the database data is written after
   this structure, and pointers contain offsets from the beginning of
   the structure. */
typedef struct {
  int ctime, mtime;
  int date1, date2;
  char * title,
       * body,
       * people,
       * route,
       * owner;
  int ntags;  /* number of int tags */
  int * tags;
} event_t;

/* event_get is used in link/geo operations to check event owner*/
int event_get(int id, event_t *ev);

#endif
