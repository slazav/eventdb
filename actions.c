#include "actions.h"
#include "user.h"
#include "event.h"
#include "log.h"
#include "geo.h"
#include <string.h>
#include <time.h>

const char * superuser = "root";

/*********************************************************************/

int auth(const char * name, char * pwd){
  int ret;
  user_t user;

  if (strlen(name)==0) return LVL_ANON;

  if (strlen(pwd)==0) return LVL_NOAUTH;

  if (user_get(&user, name)!=0) return -1;
  if ( user.active &&
      memcmp(MD5(pwd, strlen(pwd),NULL),
                user.md5, sizeof(user.md5))==0) return user.level;
#ifdef MCCME
  sleep(1);
#endif
  fprintf(stderr, "Error: bad user/password\n");
  return -1;
}

/*********************************************************************/
/* helpers */
int
level_check(int user_level, int needed_level){
  if (user_level < needed_level){
    fprintf(stderr, "Error: permission denied\n", 
      user_level, needed_level);
    return 1;
  }
  else return 0;
}

int
get_int(const char *str, const char *name){
  int ret;
  if (strlen(str)==0) return 0;
  ret=atoi(str);
  if (ret==0){
    fprintf(stderr, "Error: bad %s: %s\n", name, str);
    return -1;
  }
  return ret;
}

unsigned int
get_uint(const char *str, const char *name){
  int ret;
  ret=atoi(str);
  if (ret==0)
    fprintf(stderr, "Error: bad %s: %s\n", name, str);
  return ret;
}

/*********************************************************************/

int
do_level_show(char * user, int level, char **argv){
  printf("%d\n", level);
  return 0;
}

int
do_root_add(char * user, int level, char **argv){
  /* Anybody can add root if it does not exist */
  char *new_pwd=argv[0];
  if ( user_get(NULL, superuser)==0 ){ /* extra check, maybe not needed */
    fprintf(stderr, "Error: superuser exists\n");
    return 1;
  }
  return user_add(superuser, new_pwd, LVL_ROOT);
}

int
do_user_add(char * user, int level, char **argv){
  if (level_check(level, LVL_ADMIN)!=0) return 1;
  return user_add(argv[0], argv[1], LVL_NORM);
}

int
do_user_del(char * user, int level, char **argv){
  char *mod_usr = argv[0];
  if (level_check(level, LVL_ROOT)!=0) return 1;
  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't delete superuser\n");
    return 1;
  }
  return user_del(mod_usr);
}

int
do_user_on(char * user, int level, char **argv){
  if (level_check(level, LVL_ADMIN)!=0) return 1;
  return user_activity_set(argv[0], 1);
}

int
do_user_off(char * user, int level, char **argv){
  char *mod_usr = argv[0];
  if (level_check(level, LVL_ADMIN)!=0) return 1;
  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't deactivate superuser\n");
    return 1;
  }
  return user_activity_set(mod_usr, 0);
}

int
do_user_level_set(char * user, int level, char **argv){
  char *mod_usr = argv[0];
  int new_level = get_int(argv[1], "level");

  if (new_level<0)  return -1;

  if (level_check(level, LVL_ADMIN)!=0) return 1;

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't change superuser level\n");
    return 1;
  }

  if (new_level >= LVL_ROOT){
    fprintf(stderr, "Error: level is too high\n");
    return 1;
  }

  if (new_level <  LVL_NORM){
    fprintf(stderr, "Error: wrong level\n");
    return 1;
  }
  return user_level_set(mod_usr, new_level);
}

int
do_user_chpwd(char * user, int level, char **argv){
  char *mod_usr = argv[0];
  char *mod_pwd = argv[1];

  if (level_check(level, LVL_ADMIN)!=0) return 1;
  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't change superuser pawssword\n");
    return 1;
  }
  return user_chpwd(mod_usr, mod_pwd);
}

int
do_user_mypwd(char * user, int level, char **argv){
  if (level_check(level, LVL_NORM)!=0) return 1;
  return user_chpwd(user, argv[0]);
}

int
do_user_list(char * user, int level, char **argv){
  return user_list(USR_SHOW_NORM);
}

int
do_user_dump(char * user, int level, char **argv){
  if (level_check(level, LVL_ROOT)!=0) return 1;
  return user_list(USR_SHOW_FULL);
}

int
do_user_show(char * user, int level, char **argv){
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
  event->date1 = get_int(argv[4], "date1"); if (event->date1<0)  return -1;
  event->date2 = get_int(argv[5], "date2"); if (event->date2<0)  return -1;

  stag = argv[6], i=0;
  while (stag && (prev = strsep(&stag, ",:; \n\t"))){
    if (i>MAX_TAGS-1){
      fprintf(stderr, "Too many tags (> %d)\n", MAX_TAGS-1);
      return 1;
    }
    if (strlen(prev)){ tags[i] = atoi(prev);
      if (tags[i]==0){
        fprintf(stderr, "Error: bad tag: %s\n", prev);
        return 1;
      }
    }
    i++;
  }
  event->tags = tags;
  event->ntags = i;
  return 0;
}

int
do_event_create(char * user, int level, char **argv){
  int tags[MAX_TAGS];
  event_t event;
  event.ctime = time(NULL);
  event.owner = user;
  return level_check(level, LVL_NOAUTH) ||
         event_parse(argv, &event, tags) ||
         event_create(&event);
}

int
do_event_edit(char * user, int level, char **argv){
  int tags[MAX_TAGS];
  event_t event;
  unsigned int id = get_uint(argv[0], "event id");
  if (id == 0)  return -1;
  return level_check(level, LVL_NOAUTH) ||
         event_check_owner(id, user) ||
         event_parse(argv+1, &event, tags) ||
         event_write(id, &event, 1);
}

int
do_event_delete(char * user, int level, char **argv){
  unsigned int id = get_uint(argv[0], "event id");
  if (id == 0)  return -1;
  return level_check(level, LVL_NOAUTH) ||
         event_check_owner(id, user) ||
         event_delete(id);
}

int
do_event_show(char * user, int level, char **argv){
  unsigned int id = get_uint(argv[0], "event id");
  if (id == 0)  return -1;
  return event_show(id);
}

int
do_event_search(char * user, int level, char **argv){
  int tags[MAX_TAGS];
  event_t event;
  return event_parse(argv+1, &event, tags) ||
         event_search(argv[0], &event);
}

/*********************************************************************/

int
do_log_new(char * user, int level, char **argv){
  log_t log;
  log.event  = atoi(argv[0]);
  log.user   = argv[1];
  log.action = argv[2];
  log.msg    = argv[3];
  return log_new(&log);
}

int
do_log_print(char * user, int level, char **argv){
  unsigned int id = atoi(argv[0]);
  return log_print(id);
}

int
do_log_tsearch(char * user, int level, char **argv){
  unsigned int t1,t2;
  t1=t2=time(NULL);
  if (strlen(argv[0])) t1=atoi(argv[0]);
  if (strlen(argv[1])) t2=atoi(argv[1]);
  return log_tsearch(t1, t2);
}






