#ifndef ACTIONS_H
#define ACTIONS_H

#include "dbs.h"

#define LVL_ROOT  100
#define LVL_ADMIN  99
#define LVL_NORM    0
#define LVL_NOAUTH -1

/* Standard actions with string parameter parsing and some
   permision checking logic. Authentication, user level and
   number of arguments are checked in the main() function. */

typedef int(action_func)(dbs_t*, char*, char **);

extern action_func do_user_check, do_root_add, do_user_add, do_user_del,
                   do_user_on, do_user_off, do_user_chpwd, do_user_chlvl,
                   do_user_mypwd, do_user_list, do_user_dump, do_user_show;

extern action_func do_event_new, do_event_put, do_event_del,
                   do_event_print, do_event_search;

#endif
