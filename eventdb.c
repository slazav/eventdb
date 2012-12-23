#include "dbs.h"
#include "actions.h"
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>


#define ACT_RO 0
#define ACT_RW 1

#define ACT_NOAUTH 0
#define ACT_AUTH 1

typedef struct {
  const char * cmd_name; /* command line name */
  action_func * func; /* action function (see actions.c) */
  int db_access;      /* ACT_RO/ACT_RW */
  int need_auth;      /* ACT_NOAU/ACT_AUTH do authentication or not */
  int add_args;       /* number of additional arguments */
  int protect_arg;    /* don't log argument (passwords) -- not working!*/
  const char * arg_names, *description;
} action_t;

const action_t actions[] = {
  {"user_check",  &do_user_check,  ACT_RO, ACT_AUTH, 0,-1,
     "no argumens", "Check that caller is active and password is ok"},
  {"root_add",    &do_root_add,    ACT_RW, ACT_NOAUTH, 1, 0,
     "<pwd>", "Add superuser (if it does not exists, for everybody)"},
  {"user_add",    &do_user_add,    ACT_RW, ACT_AUTH, 2, 1,
     "<user> <pwd>", "Add user (for root and user_edit group)"},
  {"user_del",    &do_user_del,    ACT_RW, ACT_AUTH, 1,-1,
     "<user>", "Delete user (for root only)"},
  {"user_on",     &do_user_on,     ACT_RW, ACT_AUTH, 1,-1,
     "<user>", "Activate user (for root and user_edit group)"},
  {"user_off",    &do_user_off,    ACT_RW, ACT_AUTH, 1,-1,
     "<user>", "Deactivate user (for root and user_edit group)"},
  {"user_chpwd",  &do_user_chpwd,  ACT_RW, ACT_AUTH, 2, 1,
     "<user> <pwd>","Change password (for user itself and root)"},
  {"user_list",   &do_user_list,   ACT_RO, ACT_NOAUTH, 0,-1,
     "no argumens", "List all users (with active status and groups)"},
  {"user_dump",   &do_user_dump,   ACT_RO, ACT_AUTH, 0,-1,
     "no argumens", "List all users (all information, for root only)"},
  {"user_show",   &do_user_show,   ACT_RO, ACT_NOAUTH, 1,-1,
     "<user>", "Show information about user"},
  {"group_add",   &do_group_add,   ACT_RW, ACT_AUTH, 2,-1,
     "<user> <group>", "Add user to the group (for root and user_edit group)"},
  {"group_del",   &do_group_del,   ACT_RW, ACT_AUTH, 2,-1,
     "<user> <group>", "Delete user from the group (for root and user_edit group)"},
  {"group_check", &do_group_check, ACT_RO,  ACT_NOAUTH, 1,-1,
     "<group>", "Check thet caller is in the group (no authentication!)"},
  {"group_list",  &do_group_list,  ACT_RO, ACT_NOAUTH, 1,-1,
     "<usr>", "Print all groups for specified user"},
    {NULL, NULL, 0, 0, 0, 0, NULL, NULL}};

#define MAXPWD 200
main(int argc, char **argv){
  int i;
  dbs_t dbs;
  char *user, *action;
  char pwd[MAXPWD];

  if (argc<4){
    fprintf(stderr, "Usage: eventdb <user> <pwd> <action> [parameters]\n");
    return 1;
  }

  /* common cmdline arguments: <usr> <pwd> <action> */
  user   = argv[1];
  action = argv[3];
  /* protect password in the command line */
  strncpy(pwd, argv[2], MAXPWD);
  memset(argv[2], 'x', strlen(argv[2]));

  /* find corect action */
  for (i=0; actions[i].func != NULL; i++){
    int ret, flags;
    if (strcmp(actions[i].cmd_name, action)!=0) continue;

    /* check number of additional arguments */
    if (argc-4 != actions[i].add_args){
      fprintf(stderr, "Error: incorrect arguments, must be %s\n",
        actions[i].arg_names);
      return 1;
    }

    /* open database*/
    flags = actions[i].db_access==ACT_RW? DB_CREATE:DB_RDONLY;
    if (databases_open(&dbs, flags)) exit(1);

    /* Authentication */
    ret = actions[i].need_auth ? user_check(&dbs, user, pwd) : 0;

    if (ret == 0) ret = (*actions[i].func)(&dbs, user, argc-4, argv+4);
    ret = ret || databases_close(&dbs);
    return ret;
  }

  fprintf(stderr, "Error: wrong action: %s\n", action);
  return 1;
}
