#ifndef USER_H
#define USER_H

#include "dbs.h"
#include <openssl/md5.h>

/* low-level user and group operations: no permission checking */

/* structure for user database */
typedef struct {
  int  active, level;
  unsigned char md5[MD5_DIGEST_LENGTH];
} user_t;

/* Get user structure from the database. Return 0 on success,
   DB_KEYEMPTY if user does not exists or other libdb error.
   Print error message on errors.
   If user==NULL, only check that user exists. In this case
   error message about unexistent user is supressed. */
int user_get(dbs_t *dbs, user_t * user, char * name);

/* (over)write user data in the database. Return 0 on success,
   DB_KEYEXIST if user exists and overwrite==0, or other libdb error.
   Print error message on errors. */
int user_put(dbs_t *dbs, user_t * user, char * name, int overwrite);

/* Delete user. Return 0 on success,
   DB_KEYEMPTY if user does not exists or other libdb error.
   Print error message on errors. */
int user_del(dbs_t *dbs, char * name);


/* Check that user exists and active and password is correct.
   Return 0 on success, 1 on fail.
   Print "wrong user/password" or error message on errors. */
int user_check(dbs_t *dbs, char * name, char * pwd, int level);

/* Add user. Return 0 on success, DB_KEYEXIST if user exists
   or other libdb error.
   Print error message on errors. */
int user_add(dbs_t *dbs, char * name, char * pwd, int level);

/* Change user pussword. Return 0 on success,
   DB_KEYEMPTY if user does not exists or other libdb error.
   Print error message on errors. */
int user_chpwd(dbs_t *dbs, char * name, char * pwd);

/* Set user activity. Return 0 on success,
   DB_KEYEMPTY if user does not exists or other libdb error.
   Print error message on errors. */
int user_chact(dbs_t *dbs, char * name, int act);

/* Set user level. Return 0 on success,
   DB_KEYEMPTY if user does not exists or other libdb error.
   Print error message on errors. */
int user_chlvl(dbs_t *dbs, char * name, int level);

/* List all users. Returns 0 or libdb error.
   Print error message on errors.
   mode:
   0: only names of active users
   1: all users with "active" flag
   2: dump all information (including md5) */
int user_list(dbs_t *dbs, int mode);

/* The same but for a single user*/
int user_show(dbs_t *dbs, char *name, int mode);

#endif
