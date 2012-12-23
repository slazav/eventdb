#ifndef ACTIONS_H
#define ACTIONS_H

#include "dbs.h"

typedef int(action_func)(dbs_t*, char*, int, char **);

/* Second-level actions with permmision checking logic.
   Authentication and number of arguments are checked on therd level... */

extern action_func do_user_check, do_root_add, do_user_add, do_user_del,
       do_user_on, do_user_off, do_user_chpwd,
       do_user_list, do_user_dump, do_user_show,
       do_group_add, do_group_del, do_group_check, do_group_list;

#endif
