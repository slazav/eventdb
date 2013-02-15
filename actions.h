#ifndef ACTIONS_H
#define ACTIONS_H

#include "dbs.h"

#define LVL_ROOT  100
#define LVL_ADMIN  99
#define LVL_MODER   3
#define LVL_NORM    2
#define LVL_NOAUTH  1
#define LVL_ANON    0

/* just a tmp buffer size: */
#define MAX_TAGS 1024

/* Standard actions with string parameter parsing and some
   permision checking logic. Authentication, user level and
   number of arguments are checked in the main() function. */

extern const char * superuser;

/* Check password and return user level.
   Return negative value on error (bad password, db error etc),
   zero if password is empty or user level. */
int auth(const char * name, char * pwd);

int level_check(int user_level, int needed_level);
unsigned int get_uint(const char *str, const char *name);
int get_int(const char *str, const char *name);

/* Standard action arguments: user name, user level, arguments
   Number of arguments is kept in actions structure (eventdb.c)*/
typedef int(action_func)(char*, int, char **);

extern action_func do_level_show, do_root_add, do_user_add, do_user_del,
                   do_user_on, do_user_off, do_user_chpwd, do_user_level_set,
                   do_user_mypwd, do_user_list, do_user_dump, do_user_show;

#endif
