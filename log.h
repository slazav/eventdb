#ifndef LOG_H
#define LOG_H

#include "dbs.h"

/* low-level log operations: no permission checking */

/* structure for log database */
typedef struct {
  int event;
  char * user,
       * action,
       * msg;
} log_t;

/* Write new log entry to the database, print its id (time) to stdout.
   Return 0 on success. Print error message on errors. */
int log_new(log_t * log);

/* Get log entry from the database and print it to stdout in
   xml-like format.
   Return 0 on success. Print error message on errors. */
int log_print(unsigned int id);

/* Search events by time range or event number and print them to
   stdout in xml-like format.
   Return 0 on success. Print error message on errors. */
int log_tsearch(unsigned int t1, unsigned int t2);
int log_esearch(unsigned int event);

#endif
