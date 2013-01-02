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
int user_get(user_t * user, char * name);

/* (over)write user data in the database. Return 0 on success,
   DB_KEYEXIST if user exists and overwrite==0, or other libdb error.
   Print error message on errors. */
int user_put(user_t * user, char * name, int overwrite);

/* Delete user. Return 0 on success,
   DB_KEYEMPTY if user does not exists or other libdb error.
   Print error message on errors. */
int user_del(char * name);


/* Check that user exists and active and password is correct.
   Return 0 on success, 1 on fail.
   Print "wrong user/password" or error message on errors. */
int user_check(char * name, char * pwd, int level);

/* Add user. Return 0 on success, DB_KEYEXIST if user exists
   or other libdb error.
   Print error message on errors. */
int user_add(char * name, char * pwd, int level);

/* Change user pussword. Return 0 on success,
   DB_KEYEMPTY if user does not exists or other libdb error.
   Print error message on errors. */
int user_chpwd(char * name, char * pwd);

/* Set user activity. Return 0 on success,
   DB_KEYEMPTY if user does not exists or other libdb error.
   Print error message on errors. */
int user_chact(char * name, int act);

/* Set user level. Return 0 on success,
   DB_KEYEMPTY if user does not exists or other libdb error.
   Print error message on errors. */
int user_chlvl(char * name, int level);

/* List all users. Returns 0 or libdb error.
   Print error message on errors.
   modes:
    USR_SHOW_NORM:  name:activity:level
    USR_SHOW_NAME:  list only active user names
    USR_SHOW_LEVEL: user level (-1 if user not active)
    USR_SHOW_FULL:  all information: name:activity:level:md5 */
#define USR_SHOW_NORM  0
#define USR_SHOW_NAME  1
#define USR_SHOW_LEVEL 2
#define USR_SHOW_FULL  100
int user_list(int mode);

/* Print user information (same modes) */
int user_show(char *name, int mode);

#endif
