#include "actions.h"
#include "user.h"
#include "event.h"
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
  if (ret<=0){
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

