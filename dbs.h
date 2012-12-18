#ifndef DBS_H
#define DBS_H
#include <db.h>
#include <stdio.h>

/* Structure holding all database pointers + open/close functions. */

/* see http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
 * for db docs and exmples
 */

/* directory for databases */
#ifdef MCCME
#define DEFAULT_HOMEDIR "/home/slazav/eventdb/dbs"
#else
#define DEFAULT_HOMEDIR "./dbs"
#endif

typedef struct {
 DB * events;
 DB * users;
 DB * groups;
} dbs_t;

/* Open/close all databases. Print msg to stderr and return
   non-zero code if failed.
   Open flags must be DB_CREATE or DB_RDONLY */
int databases_open(dbs_t *, int flags);
int databases_close(dbs_t *);

#endif
