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
    int ret;

    /* Open the event database */
    ret = open_database(&(dbs->events), DEFAULT_HOMEDIR "/events",
                        stderr, flags, 0);
    if (ret != 0) return ret;

    /* Open the user database */
    ret = open_database(&(dbs->users), DEFAULT_HOMEDIR "/users",
                        stderr, flags, 0);
    if (ret != 0) return ret;

    /* Open the group database */
    ret = open_database(&(dbs->groups), DEFAULT_HOMEDIR "/groups",
                        stderr, flags, 1);
    if (ret != 0) return ret;

    return (0);
}

/* Closes all the databases. */
int
databases_close(dbs_t *dbs){
    int ret;

    if (dbs->events != NULL) {
        ret = dbs->events->close(dbs->events, 0);
        if (ret != 0)
            fprintf(stderr, "Error: event database close failed: %s\n",
               db_strerror(ret));
    }

    if (dbs->users != NULL) {
        ret = dbs->users->close(dbs->users, 0);
        if (ret != 0)
            fprintf(stderr, "Error: user database close failed: %s\n",
              db_strerror(ret));
    }

    if (dbs->groups != NULL) {
        ret = dbs->groups->close(dbs->groups, 0);
        if (ret != 0)
            fprintf(stderr, "Error: groups database close failed: %s\n",
              db_strerror(ret));
    }

    return (0);
}
