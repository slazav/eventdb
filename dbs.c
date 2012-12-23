#include "dbs.h"
#include <string.h>
#include <stdlib.h>

/* Opens a database */
int
open_database(DB **dbpp,       /* The DB handle that we are opening */
    const char *file_name,     /* The file in which the db lives */
    FILE *error_file_pointer,  /* File where we want error messages sent */
    int flags, int dup)
{
    DB *dbp;    /* For convenience */
    int ret;

    /* Initialize the DB handle */
    ret = db_create(&dbp, NULL, 0);
    if (ret != 0) {
        fprintf(error_file_pointer, "%s\n", db_strerror(ret));
        return(ret);
    }

    /* Point to the memory malloc'd by db_create() */
    *dbpp = dbp;

    /* Set up error handling for this database */
    /* Not needed? */
    dbp->set_errfile(dbp, error_file_pointer);

    if (dup){
      ret = dbp->set_flags(dbp, DB_DUPSORT);
      if (ret != 0) {
        fprintf(stderr, "Error: can't set DB_DUPSORT flag:%s: %s\n",
          file_name, db_strerror(ret));
        dbp->close(dbp, 0);
        return(ret);
      }
    }

    /* Now open the database */
    ret = dbp->open(dbp,        /* Pointer to the database */
                    NULL,       /* Txn pointer */
                    file_name,  /* File name */
                    NULL,       /* Logical db name (unneeded) */
                    DB_BTREE,   /* Database type (using btree) */
                    flags,      /* Open flags */
                    0);         /* File mode. Using defaults */
    if (ret != 0) {
      fprintf(stderr, "Error: can't open database: %s: %s\n",
          file_name, db_strerror(ret));
      return(ret);
    }

    return (0);
}

/* opens all databases */
int
databases_open(dbs_t *dbs, int flags){
  return
    open_database(&(dbs->users),  DEFAULT_HOMEDIR "/users",  stderr, flags, 0) ||
    open_database(&(dbs->log),    DEFAULT_HOMEDIR "/log",    stderr, flags, 0) ||
    open_database(&(dbs->events), DEFAULT_HOMEDIR "/events", stderr, flags, 0) ||
    open_database(&(dbs->links),  DEFAULT_HOMEDIR "/links",  stderr, flags, 1) ||
    open_database(&(dbs->tracks), DEFAULT_HOMEDIR "/tracks", stderr, flags, 1) ||
    0;
}

int
close_database(DB *dbp){ /* The DB handle that we are opening */
  int ret;
  if (dbp == NULL) return 0;
  ret = dbp->close(dbp, 0);
  if (ret != 0)
    fprintf(stderr, "Error: database close failed: %s\n",
      db_strerror(ret));
  return ret;
}

/* Closes all the databases. */
int
databases_close(dbs_t *dbs){
    int ret = 0;
    ret = close_database(dbs->users)  || ret;
    ret = close_database(dbs->events) || ret;
    ret = close_database(dbs->links)  || ret;
    ret = close_database(dbs->tracks) || ret;
    return ret;
}

/* create DBT object of various type */

DBT mk_empty_dbt(){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  return ret;
}

DBT mk_int_dbt(int * i){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  ret.data = i;
  ret.size = sizeof(int);
  return ret;
}

DBT mk_string_dbt(char * str){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  ret.data = str;
  ret.size = strlen(str)+1;
  return ret;
}
