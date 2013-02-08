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
  int add_args;       /* number of additional arguments */
  const char * arg_names, *description;
} action_t;

const action_t actions[] = {
  {"level_show",  &do_level_show,   ACT_RO, 0,
     "-", "Check password/activity and print level"},
  {"root_add",    &do_root_add,    ACT_RW, 1,
     "<pwd>", "Add superuser if it does not exists"},
  {"user_add",    &do_user_add,    ACT_RW, 2,
     "<user> <pwd>", "Add user"},
  {"user_del",    &do_user_del,    ACT_RW, 1,
     "<user>", "Delete user"},
  {"user_on",     &do_user_on,     ACT_RW, 1,
     "<user>", "Activate user"},
  {"user_off",    &do_user_off,    ACT_RW, 1,
     "<user>", "Deactivate non-root user"},
  {"user_level_set",  &do_user_level_set,  ACT_RW, 2,
     "<user> <pwd>","Change non-root level"},
  {"user_chpwd",  &do_user_chpwd,  ACT_RW, 2,
     "<user> <pwd>","Change non-root password"},
  {"user_mypwd",  &do_user_mypwd,  ACT_RW, 1,
     "<pwd>","Change caller's password"},
  {"user_list",   &do_user_list,   ACT_RO, 0,
     "-", "List all users with active status and levels"},
  {"user_dump",   &do_user_dump,   ACT_RO, 0,
     "-", "List all info about users"},
  {"user_show",   &do_user_show,   ACT_RO, 1,
     "<user>", "Show information about user"},

  {"event_create",   &do_event_create,   ACT_RW, 7,
     "<title> <body> <people> <route> <date1> <date2> <tags>",
     "Add new event, print its id"},
  {"event_edit",   &do_event_edit,   ACT_RW, 8,
     "<id> <title> <body> <people> <route> <date1> <date2> <tags>",
     "Add new event, print its id"},
  {"event_delete",   &do_event_delete,   ACT_RW, 1,
     "<id>", "Delete event"},
  {"event_show", &do_event_show, ACT_RO, 1,
     "<id>", "Print event data"},
  {"event_search",&do_event_search,ACT_RO, 8,
     "<text> <title> <body> <people> <route> <date1> <date2> <tags>",
     "Find events: use <text> for full text search or special event fields"},

  {"log_new",   &do_log_new,   ACT_RW, 4,
     "<event> <usr> <action> <msg>",
     "Add new log, print its id"},
  {"log_print", &do_log_print, ACT_RO, 1,
     "<id>", "Print log entry"},
  {"log_tsearch", &do_log_tsearch, ACT_RO, 2,
     "<t1> <t2>", "Print all log entries between t1 and t2. Use \"\" for now"},

  {"geo_create",   &do_geo_create,   ACT_RW, 7,
     "<fname> <comm> <auth> <date1> <date2> <length> <tags>",
     "Add new geodata file"},
  {"geo_edit",     &do_geo_edit,     ACT_RW, 7,
     "<fname> <comm> <auth> <date1> <date2> <length> <tags>",
     "Edit geodata information"},
  {"geo_delete",   &do_geo_delete,   ACT_RW, 1,
     "<fname>",
     "Delete geodata"},
  {"geo_replace",   &do_geo_replace,   ACT_RW, 1,
     "<fname>",
     "Delete geodata"},
  {"geo_show", &do_geo_show, ACT_RO, 1,
     "<fname>", "Print geo data"},
  {"geo_list", &do_geo_list, ACT_RO, 0,
     "<fname>", "Print all geo data"},

  {NULL, NULL, 0, 0, NULL, NULL}};

void
print_short_help(){
  fprintf(stderr, "Usage: eventdb <user> <pwd> <action> [action args]\n");
  fprintf(stderr, "       eventdb -h \n");
}

#define MAXPWD 200
main(int argc, char **argv){
  int i;
  char *user, *action;
  char pwd[MAXPWD];

  /* print help message*/
  if (argc==2 && strcmp(argv[1], "-h")==0){
    print_short_help();
    fprintf(stderr, "Actions:\n");
    for (i=0; actions[i].func != NULL; i++){
      fprintf(stderr, "%14s -- %s ",
        actions[i].cmd_name, actions[i].description);
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
    int ret, flags, level;
    if (strcmp(actions[i].cmd_name, action)!=0) continue;

    /* check number of additional arguments */
    if (argc-4 != actions[i].add_args){
      fprintf(stderr, "Error: incorrect arguments, must be %s\n",
        actions[i].arg_names);
      exit(1);
    }

    /* open database */
    flags = actions[i].db_access==ACT_RW? DB_CREATE:DB_RDONLY;
    if (databases_open(flags)!=0) exit(1);

    /* Find user level */
    level=auth(user, pwd);
    if (level<0) exit(1);

    /* do action */
    ret = (*actions[i].func)(user, level, argv+4);

    /* close databases */
    ret = databases_close() || ret;
    exit(ret);
  }

  fprintf(stderr, "Error: wrong action: %s\n", action);
  exit(1);
}
