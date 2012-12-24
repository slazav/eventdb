#include "dbs.h"
#include "actions.h"
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>


#define ACT_RO 0
#define ACT_RW 1

typedef struct {
  const char * cmd_name; /* command line name */
  action_func * func; /* action function (see actions.c) */
  int db_access;      /* ACT_RO/ACT_RW */
  int level;          /* LVL_NOAUTH, LVL_NORM, LVL_ADMIN, LVL_ROOT */
  int add_args;       /* number of additional arguments */
  const char * arg_names, *description;
} action_t;

const action_t actions[] = {
  {"user_check",  &do_user_check,  ACT_RO, LVL_NORM, 0,
     "-", "Check that caller is active and password is ok"},
  {"root_add",    &do_root_add,    ACT_RW, LVL_NOAUTH, 1,
     "<pwd>", "Add superuser if it does not exists"},
  {"user_add",    &do_user_add,    ACT_RW, LVL_ADMIN, 2,
     "<user> <pwd>", "Add user"},
  {"user_del",    &do_user_del,    ACT_RW, LVL_ROOT, 1,
     "<user>", "Delete user"},
  {"user_on",     &do_user_on,     ACT_RW, LVL_ADMIN, 1,
     "<user>", "Activate user"},
  {"user_off",    &do_user_off,    ACT_RW, LVL_ADMIN, 1,
     "<user>", "Deactivate non-root user"},
  {"user_chlvl",  &do_user_chlvl,  ACT_RW, LVL_ADMIN, 2,
     "<user> <pwd>","Change non-root level"},
  {"user_chpwd",  &do_user_chpwd,  ACT_RW, LVL_ADMIN, 2,
     "<user> <pwd>","Change non-root password"},
  {"user_mypwd",  &do_user_mypwd,  ACT_RW, LVL_NORM, 1,
     "<pwd>","Change caller's password"},
  {"user_list",   &do_user_list,   ACT_RO, LVL_NOAUTH, 0,
     "-", "List all users with active status and levels"},
  {"user_dump",   &do_user_dump,   ACT_RO, LVL_ROOT, 0,
     "-", "List all info about users"},
  {"user_show",   &do_user_show,   ACT_RO, LVL_NOAUTH, 1,
     "<user>", "Show information about user"},
  {"event_new",   &do_event_new,   ACT_RW, LVL_NORM, 7,
     "<title> <body> <people> <route> <date1> <date2> <tags>",
     "Add new event, print its id"},
  {"event_put",   &do_event_put,   ACT_RW, LVL_NORM, 8,
     "<id> <title> <body> <people> <route> <date1> <date2> <tags>",
     "Add new event, print its id"},
  {"event_del",   &do_event_del,   ACT_RW, LVL_NORM, 1,
     "<id>", "Delete event"},
  {"event_print", &do_event_print, ACT_RO, LVL_NOAUTH, 1,
     "<id>", "Print event data"},
  {"event_search",&do_event_search,ACT_RO, LVL_NOAUTH, 7,
     "<title> <body> <people> <route> <date1> <date2> <tags>",
     "Find events"},
  {NULL, NULL, 0, 0, 0, NULL, NULL}};

void
print_short_help(){
  fprintf(stderr, "Usage: eventdb <user> <pwd> <action> [action args]\n");
  fprintf(stderr, "       eventdb -h \n");
}

#define MAXPWD 200
main(int argc, char **argv){
  int i;
  dbs_t dbs;
  char *user, *action;
  char pwd[MAXPWD];

  /* print help message*/
  if (argc==2 && strcmp(argv[1], "-h")==0){
    print_short_help();
    fprintf(stderr, "Actions:\n");
    for (i=0; actions[i].func != NULL; i++){
      fprintf(stderr, "%14s -- %s ",
        actions[i].cmd_name, actions[i].description);
      switch (actions[i].level){
        case LVL_NOAUTH: fprintf(stderr, "(everybody)"); break;
        case LVL_NORM:   fprintf(stderr, "(any user)"); break;
        case LVL_ADMIN:  fprintf(stderr, "(admin)"); break;
        case LVL_ROOT:   fprintf(stderr, "(root)"); break;
        default:   fprintf(stderr, "(\?\?\?)");
      }
      fprintf(stderr, "\n");
      fprintf(stderr, "%14s    %d args: %s\n", "",
        actions[i].add_args, actions[i].arg_names);
    }
    exit(1);
  }
  if (argc<4){
    print_short_help();
    exit(1);
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
      exit(1);
    }

    /* open database*/
    flags = actions[i].db_access==ACT_RW? DB_CREATE:DB_RDONLY;
    if (databases_open(&dbs, flags)) exit(1);

    /* Authentication and level checking */
    ret = user_check(&dbs, user, pwd, actions[i].level);

    if (ret == 0) ret = (*actions[i].func)(&dbs, user, argv+4);
    ret = ret || databases_close(&dbs);
    exit(ret);
  }

  fprintf(stderr, "Error: wrong action: %s\n", action);
  exit(1);
}
