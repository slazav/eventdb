#include "actions.h"
#include "user.h"
#include "event.h"
#include "log.h"
#include <string.h>
#include <time.h>

char * superuser = "root";

/* just a tmp buffer size: */
#define MAX_TAGS 1024

/*********************************************************************/

int
do_user_check(char * user, char **argv){
  /* auth check is performed outside */
  return user_show(user, USR_SHOW_LEVEL);
}

int
do_root_add(char * user, char **argv){
  char *new_pwd=argv[0];
  if ( user_get(NULL, superuser)==0 ){ /* extra check, maybe not needed */
    fprintf(stderr, "Error: superuser exists\n");
    return 1;
  }
  return user_add(superuser, new_pwd, LVL_ROOT);
}

int
do_user_add(char * user, char **argv){
  return user_add(argv[0], argv[1], LVL_NORM);
}

int
do_user_del(char * user, char **argv){
  char *mod_usr = argv[0];

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't delete superuser\n");
    return 1;
  }
  return user_del(mod_usr);
}

int
do_user_on(char * user, char **argv){
  return user_chact(argv[0], 1);
}

int
do_user_off(char * user, char **argv){
  char *mod_usr = argv[0];

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't deactivate superuser\n");
    return 1;
  }
  return user_chact(mod_usr, 0);
}

int
do_user_chlvl(char * user, char **argv){
  char *mod_usr = argv[0];
  int level = atoi(argv[1]);

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't change superuser level\n");
    return 1;
  }
  if (level >= LVL_ROOT){
    fprintf(stderr, "Error: level is too high\n");
    return 1;
  }
  if (level <  LVL_NORM){
    fprintf(stderr, "Error: wrong level\n");
    return 1;
  }
  return user_chlvl(mod_usr, level);
}

int
do_user_chpwd(char * user, char **argv){
  char *mod_usr = argv[0];
  char *mod_pwd = argv[1];

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't change superuser pawssword\n");
    return 1;
  }
  return user_chpwd(mod_usr, mod_pwd);
}

int
do_user_mypwd(char * user, char **argv){
  return user_chpwd(user, argv[0]);
}

int
do_user_list(char * user, char **argv){
  return user_list(USR_SHOW_NORM);
}

int
do_user_dump(char * user, char **argv){
  return user_list(USR_SHOW_FULL);
}

int
do_user_show(char * user, char **argv){
  return user_show(argv[0], USR_SHOW_NORM);
}

/*********************************************************************/

/* get event information from argv[0..6] and put it to event and tags */
int
event_parse(char **argv, event_t * event, int tags[MAX_TAGS]){
  char *stag, *prev;
  int i;

  event->title  = argv[0];
  event->body   = argv[1];
  event->people = argv[2];
  event->route  = argv[3];
  event->date1 = atoi(argv[4]);
  if (event->date1==0){
    fprintf(stderr, "Error: bad date1: %s\n", argv[4]);
    return 1;
  }
  event->date2 = atoi(argv[5]);
  if (event->date2==0){
    fprintf(stderr, "Error: bad date2: %s\n", argv[5]);
    return 1;
  }
  stag = argv[6], i=0;
  while (stag && (prev = strsep(&stag, ",:; \n\t"))){
    if (i>MAX_TAGS-1){
      fprintf(stderr, "Too many tags (> %d)\n", MAX_TAGS-1);
      return 1;
    }
    if (strlen(prev)) tags[i] = atoi(prev);
    if (tags[i]==0){
      fprintf(stderr, "Error: bad tag: %s\n", prev);
      return 1;
    }
    i++;
  }
  event->tags = tags;
  event->ntags = i;
  return 0;
}

int
do_event_new(char * user, char **argv){
  int tags[MAX_TAGS];
  event_t event;
  return event_parse(argv, &event, tags) ||
         event_new(&event);
}

int
do_event_put(char * user, char **argv){
  int tags[MAX_TAGS];
  event_t event;
  unsigned int id = atoi(argv[0]);
  if (id==0){
    fprintf(stderr, "Error: bad event id: %s\n", argv[0]);
    return 1;
  }
  return event_parse(argv+1, &event, tags) ||
         event_put(id, &event, 1);
}

int
do_event_del(char * user, char **argv){
  unsigned int id = atoi(argv[0]);
  return event_del(id);
}

int
do_event_print(char * user, char **argv){
  unsigned int id = atoi(argv[0]);
  return event_print(id);
}

int
do_event_search(char * user, char **argv){
  int tags[MAX_TAGS];
  event_t event;
  return event_parse(argv, &event, tags) ||
         event_search(&event);
}

/*********************************************************************/

int
do_log_new(char * user, char **argv){
  log_t log;
  log.event  = atoi(argv[0]);
  log.user   = argv[1];
  log.action = argv[2];
  log.msg    = argv[3];
  return log_new(&log);
}

int
do_log_print(char * user, char **argv){
  unsigned int id = atoi(argv[0]);
  return log_print(id);
}

int
do_log_tsearch(char * user, char **argv){
  unsigned int t1,t2;
  t1=t2=time(NULL);
  if (strlen(argv[0])) t1=atoi(argv[0]);
  if (strlen(argv[1])) t2=atoi(argv[1]);
  return log_tsearch(t1, t2);
}
