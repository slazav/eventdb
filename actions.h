#ifndef ACT_H
#define ACT_H

#include "err.h"
#include "cfg.h"
/*
  Actions.
*/

void do_login(const CFG & cfg, int argc, char **argv);
void do_user_info(const CFG & cfg, int argc, char **argv);
void do_logout(const CFG & cfg, int argc, char **argv);
void do_set_alias(const CFG & cfg, int argc, char **argv);
void do_set_abbr(const CFG & cfg, int argc, char **argv);

#endif
