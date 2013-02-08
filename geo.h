#ifndef GEO_H
#define GEO_H

#include "dbs.h"

#ifdef MCCME
#define GEODIR "/home/slazav/CH/gps"
#else
#define GEODIR "./gps"
#endif

#define MAXFNAME 32
#define MAXFSIZE 100000
#define BUFLEN 8192



/* Low-level geodata operations: no permission checking.
   When link is recorded to the database all <html tags>
   are converted to [html tags] and '\n' is converted to ' '.
   Print and search queries print events in xml-like format.
*/

/* Geodata structure. In the database data is written after
   this structure, and pointers contain offsets from the beginning of
   the structure. */
typedef struct {
  int ctime;
  int date1, date2;
  int length;
  char * comm,
       * auth,
       * owner;
  int ntags;  /* number of int tags */
  int * tags;
} geo_t;

/* Create new file from STDIN and write metadata
   Fail if file exists.
   Return 0 on success. Print error message on errors. */
int geo_create(char * fname, geo_t * geo);

/* Replace existing file from STDIN (metadata must exist) */
int geo_replace(char * fname);

/* Delete file and metadata (metadata must exist) */
int geo_delete(char * fname);

/* Edit (or add) metadata for existing file */
int geo_edit(char * fname, geo_t * geo);

/* Check owner */
int geo_check_owner(char * fname, char *user);

/* Show metadata for a filename */
int geo_show(char * fname);

/* Show metadata for a filename */
int geo_list();

#endif
