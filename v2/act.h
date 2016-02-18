#ifndef ACT_H
#define ACT_H

#include "err.h"
#include "cfg.h"
/*
  Actions.
*/

/* Check argument number */
//void check_nargs(int argc, int n){
//  if (argc-1!=n) throw Err("eventdb_action")
//    << "wrong number of arguments, should be " << n;
//}

void do_login(const CFG & cfg, int argc, char **argv);
void do_logout(const CFG & cfg, int argc, char **argv);
void do_user_info(const CFG & cfg, int argc, char **argv);

#endif
