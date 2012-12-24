#include "actions.h"
#include "user.h"
#include "event.h"
#include <string.h>

char * superuser = "root";

/* just a tmp buffer size: */
#define MAX_TAGS 1024

/*********************************************************************/

int
do_user_check(dbs_t * dbs, char * user, char **argv){
  /* auth check is performed outside */
  return user_show(dbs, user, USR_SHOW_LEVEL);
}

int
do_root_add(dbs_t * dbs, char * user, char **argv){
  char *new_pwd=argv[0];
  if ( user_get(dbs, NULL, superuser)==0 ){ /* extra check, maybe not needed */
    fprintf(stderr, "Error: superuser exists\n");
    return 1;
  }
  return user_add(dbs, superuser, new_pwd, LVL_ROOT);
}

int
do_user_add(dbs_t * dbs, char * user, char **argv){
  return user_add(dbs, argv[0], argv[1], LVL_NORM);
}

int
do_user_del(dbs_t * dbs, char * user, char **argv){
  char *mod_usr = argv[0];

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't delete superuser\n");
    return 1;
  }
  return user_del(dbs, mod_usr);
}

int
do_user_on(dbs_t * dbs, char * user, char **argv){
  return user_chact(dbs, argv[0], 1);
}

int
do_user_off(dbs_t * dbs, char * user, char **argv){
  char *mod_usr = argv[0];

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't deactivate superuser\n");
    return 1;
  }
  return user_chact(dbs, mod_usr, 0);
}

int
do_user_chlvl(dbs_t * dbs, char * user, char **argv){
  char *mod_usr = argv[0];
  int level = atoi(argv[1]);

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't change superuser level\n");
    return 1;
  }

  if (level >= LVL_ROOT) level = LVL_ROOT-1; /* can't set root level! */
  if (level <  LVL_NORM) level = LVL_NORM;
  return user_chlvl(dbs, mod_usr, level);
}

int
do_user_chpwd(dbs_t * dbs, char * user, char **argv){
  char *mod_usr = argv[0];
  char *mod_pwd = argv[1];

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't change superuser pawssword\n");
    return 1;
  }
  return user_chpwd(dbs, mod_usr, mod_pwd);
}

int
do_user_mypwd(dbs_t * dbs, char * user, char **argv){
  return user_chpwd(dbs, user, argv[0]);
}

int
do_user_list(dbs_t * dbs, char * user, char **argv){
  return user_list(dbs, USR_SHOW_NORM);
}

int
do_user_dump(dbs_t * dbs, char * user, char **argv){
  return user_list(dbs, USR_SHOW_FULL);
}

int
do_user_show(dbs_t * dbs, char * user, char **argv){
  return user_show(dbs, argv[0], USR_SHOW_NORM);
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
do_event_new(dbs_t * dbs, char * user, char **argv){
  int tags[MAX_TAGS];
  event_t event;
  return event_parse(argv, &event, tags) ||
         event_new(dbs, &event);
}

int
do_event_put(dbs_t * dbs, char * user, char **argv){
  int tags[MAX_TAGS];
  event_t event;
  int id = atoi(argv[0]);
  if (id==0){
    fprintf(stderr, "Error: bad event id: %s\n", argv[0]);
    return 1;
  }
  return event_parse(argv+1, &event, tags) ||
         event_put(dbs, id, &event, 1);
}

int
do_event_del(dbs_t * dbs, char * user, char **argv){
  int id = atoi(argv[0]);
  return event_del(dbs, id);
}

int
do_event_print(dbs_t * dbs, char * user, char **argv){
  int id = atoi(argv[0]);
  return event_print(dbs, id);
}

int
do_event_search(dbs_t * dbs, char * user, char **argv){
  int tags[MAX_TAGS];
  event_t event;
  return event_parse(argv, &event, tags) ||
         event_search(dbs, &event);
}

/*********************************************************************/
