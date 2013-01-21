#ifndef EVENT_H
#define EVENT_H

#include "dbs.h"

/* Low-level event operations: no permission checking.
   When event is recorded to the database all <html tags>
   are converted to [html tags] and '\n' is converted to ' '
   in all fields but body.
   Print and search queries print events in an xml-like format.
*/

/* Event structure. In the database data is written after
   this structure, and pointers contain offsets from the beginning of
   the structure. */
typedef struct {
  int date1, date2;
  int ntags;  /* number of int tags */
  char * title,
       * body,
       * people,
       * route;
  int * tags;
} event_t;

/* Write event to the database.
   Return 0 on success. Print error message on errors. */
int event_put(unsigned int id, event_t * event, int overwrite);

/* Write new event to the database, print id to stdout.
   Return 0 on success. Print error message on errors. */
int event_new(event_t * event);

/* Delete event from the database.
   Return 0 on success. Print error message on errors. */
int event_del(unsigned int id);

/* Get event from the database and print it to stdout in
   xml-like format.
   Return 0 on success. Print error message on errors. */
int event_print(unsigned int id);

/* Search events corresponding to the mask and print them to
   stdout in xml-like format.
   Use date1=-1
   Return 0 on success. Print error message on errors. */
int event_search(const char * txt, event_t * mask);


#endif
