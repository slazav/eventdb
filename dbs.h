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
 DB * users;
 DB * logs;
 DB * events;
 DB * links;
 DB * tracks;
} dbs_t;

/* everybody uses this global var! */
extern dbs_t dbs;

/* Open/close all databases. Print msg to stderr and return
   non-zero code if failed.
   Open flags must be DB_CREATE or DB_RDONLY */
int databases_open(int flags);
int databases_close();

/* create DBT for some simple data types*/
DBT mk_empty_dbt();
DBT mk_uint_dbt(unsigned int * i);
DBT mk_string_dbt(const char * str);

/* comparison funtion for unsigned int keys */
int compare_uint(DB *dbp, const DBT *a, const DBT *b);

/*********************************************************************/
/* Replace '<' and '>' by '[' and ']' in the string.
   Replace '\n' by ' ' if rem_nl!=0 */
# define REMOVE_NL 0
# define KEEP_NL   1
void remove_html(char * str, int rem_nl);

#endif
