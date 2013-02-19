#include "dbs.h"
#include <string.h>
#include <stdlib.h>

#define NODUP 0
#define DUP   1

#define DBMASK_USERS  1
#define DBMASK_LOGS   2
#define DBMASK_EVENTS 4
#define DBMASK_LINKS  8
#define DBMASK_TRACKS 16

dbs_t dbs;

typedef struct {
  long unsigned int mask; /* mask */
  DB **dbpp;              /* pointer to the DB handle in dbs */
  const char *file;       /* The file in which the db lives */
  int dup;                /* allow duplicated entries */
  int (*cmp_func)(DB *db, const DBT *dbt1, const DBT *dbt2); /* comparison function */
} database_t;

const database_t databases[] = {
  {DBMASK_USERS,  &(dbs.users),  DEFAULT_HOMEDIR "/users",  NODUP, NULL},
  {DBMASK_LOGS,   &(dbs.logs),   DEFAULT_HOMEDIR "/logs",   NODUP, compare_uint},
  {DBMASK_EVENTS, &(dbs.events), DEFAULT_HOMEDIR "/events", NODUP, compare_uint},
  {DBMASK_EVENTS, &(dbs.d2ev),   DEFAULT_HOMEDIR "/d2ev",   DUP, compare_uint},
  {DBMASK_LINKS,  &(dbs.links),  DEFAULT_HOMEDIR "/links",  DUP, compare_uint},
  {DBMASK_TRACKS, &(dbs.tracks), DEFAULT_HOMEDIR "/tracks", DUP, compare_uint},
  {0,NULL,NULL,0,NULL}
};

int db_event_date1(DB *secdb, const DBT *pkey, const DBT *pdata, DBT *skey);

int
databases_open(int flags){
  database_t *db;
  int i;
  for (i=0; databases[i].mask; i++){
    DB *dbp;    /* For convenience */
    int ret;

    if (*databases[i].dbpp) continue;

    /* Initialize the DB handle */
    ret = db_create(&dbp, NULL, 0);
    if (ret != 0) {
        fprintf(stderr, "%s\n", db_strerror(ret));
        return(ret);
    }

    /* Point to the memory malloc'd by db_create() */
    *databases[i].dbpp = dbp;

    /* setup key duplication if needed */
    if (databases[i].dup){
      ret = dbp->set_flags(dbp, DB_DUPSORT);
      if (ret != 0) {
        fprintf(stderr, "Error: can't set DB_DUPSORT flag: %s: %s\n",
          databases[i].file, db_strerror(ret));
        dbp->close(dbp, 0);
        return(ret);
      }
    }

    /* set key compare function if needed */
    if (databases[i].cmp_func){
      ret = dbp->set_bt_compare(dbp, databases[i].cmp_func);
      if (ret != 0) {
        fprintf(stderr, "Error: can't set compare func: %s: %s\n",
          databases[i].file, db_strerror(ret));
        dbp->close(dbp, 0);
        return(ret);
      }
    }

    /* Now open the database */
    ret = dbp->open(dbp,        /* Pointer to the database */
                    NULL,       /* Txn pointer */
                    databases[i].file,   /* File name */
                    NULL,       /* Logical db name (unneeded) */
                    DB_BTREE,   /* Database type (using btree) */
                    flags,      /* Open flags */
                    0);         /* File mode. Using defaults */
    if (ret != 0) {
      fprintf(stderr, "Error: can't open database: %s: %s\n",
          databases[i].file, db_strerror(ret));
      return(ret);
    }

  }
  /* associate secondary dbs (maybe it is better to
     put it to the main loop) */
  dbs.d2ev->associate(dbs.events, NULL, dbs.d2ev, db_event_date1, 0);
}

int
databases_close(){
  database_t *db;
  int i, ret, ret1=0;
  for (i=0; databases[i].mask; i++)/* go to the end */;
  for (i--; i>=0; i--){
    if (*databases[i].dbpp == NULL) continue;
    ret = (*databases[i].dbpp)->close(*databases[i].dbpp, 0);
    if (ret != 0)
      fprintf(stderr, "Error: database close failed: %s\n",
        db_strerror(ret));
    ret1 += (ret!=0);
  }
  return ret1;
}



/* create DBT object of various type */

DBT mk_empty_dbt(){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  return ret;
}

DBT mk_uint_dbt(unsigned int * i){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  ret.data = i;
  ret.size = sizeof(int);
  return ret;
}

DBT mk_string_dbt(const char * str){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  ret.data = str;
  ret.size = strlen(str)+1;
  return ret;
}

int
compare_uint(DB *dbp, const DBT *a, const DBT *b){
    unsigned int ai, bi;
    memcpy(&ai, a->data, sizeof(int));
    memcpy(&bi, b->data, sizeof(int));
    return (ai - bi);
}

/*********************************************************************/

void
remove_html(char * str, int rem_nl){
  int i;
  for (i=0; i<strlen(str); i++){
    if (str[i]=='<') str[i]='[';
    if (str[i]=='>') str[i]=']';
    if (rem_nl && str[i]=='\n') str[i]=' ';
  }
}

