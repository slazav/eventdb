#ifndef ACT_H
#define ACT_H

#include "err.h"
#include "cfg.h"
/*
  Actions.
*/

/* Standard action arguments: CFG, argc, argv */
typedef void(action_func)(const CFG &, int, char **);

action_func do_login,
            do_my_info,
            do_logout,
            do_set_alias,
            do_set_level,
            do_user_list,
            do_joinreq_add,
            do_joinreq_delete,
            do_joinreq_accept;

action_func do_ev_new,
            do_ev_del,
            do_ev_edit,
            do_ev_show,
            do_ev_list;

#endif
