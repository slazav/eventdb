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

/* superuser name (root) */
extern const char * superuser;

/* Standard actions with string parameter parsing and some
   permision checking logic. Authentication, user level and
   number of arguments are checked in the main() function. */

/* Check password and return user level.
   Return negative value on error (bad password, db error etc),
   zero if password is empty or user level. */
int auth(const char * name, char * pwd);

int level_check(int user_level, int needed_level);
int get_int(const char *str, const char *name);
int get_tags(char *str, int tags[MAX_TAGS]);

/* Standard action arguments: user name, user level, arguments
   Number of arguments is kept in actions structure (eventdb.c)*/
typedef int(action_func)(char*, int, char **);

/* print links for one event (see link.h) */
int list_event_links(int eventid);

extern action_func do_level_show, do_root_add, do_user_add, do_user_del,
                   do_user_activ_set, do_user_level_set, do_user_chpwd,
                   do_user_mypwd, do_user_list, do_user_dump, do_user_show;

extern action_func do_event_create, do_event_edit, do_event_delete,
                   do_event_show, do_event_list, do_event_search;

extern action_func do_link_create, do_link_edit, do_link_delete, do_link_replace,
                   do_link_show, do_link_list, do_link_list_ev, do_link_search;

extern action_func do_log_new, do_log_print, do_log_tsearch;


#endif
