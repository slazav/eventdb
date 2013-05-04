#ifndef EVENT_H
#define EVENT_H
#include "actions.h"
#include <stdint.h>

/*********************************************************************/
typedef union {char    * ptr; uint64_t off;} char_ptr;
typedef union {int32_t * ptr; uint64_t off;} int_ptr;

/* Event structure. In the database data is written after
   this structure, and pointers contain offsets from the beginning of
   the structure.
   Don't put here arch-dependent things such as int or char*! */
typedef struct {
  int32_t ctime, mtime;
  int32_t date1, date2;
  char_ptr title,
           body,
           people,
           route,
           owner;
  int32_t ntags;  /* number of int tags */
  int_ptr tags;
} event_t;

/* event_get is used in link/geo operations to check event owner*/
int event_get(int id, event_t *ev);

#endif
