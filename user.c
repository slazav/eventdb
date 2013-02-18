#include "actions.h"
#include <openssl/md5.h>
#include <string.h>

/****************************************************************/

/* max username length, min password length */
#define MAXNAME 30
#define MINPASS 4

/* structure for user database */
typedef struct {
  int  active, level;
  unsigned char md5[MD5_DIGEST_LENGTH];
} user_t;

const char * superuser = "root";

/****************************************************************/

int
check_name(const char * name){
  const char accept[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789-_";
  int i, n=strlen(name);
  if (n<1){
    fprintf(stderr, "Error: empty name\n");
    return -1;
  }
  if (n>MAXNAME){
    fprintf(stderr, "Error: too long name (>%d chars)\n", MAXNAME);
    return -1;
  }
  if (strspn(name, accept)!=n){
    fprintf(stderr, "Error: only a-z,A-Z,0-9, and _ characters are accepted\n");
    return -1;
  }
  return 0;
}

/****************************************************************/

int
user_get(user_t * user, const char * name){
  int ret;
  DBT key = mk_string_dbt(name);
  DBT val = mk_empty_dbt();

  ret = dbs.users->get(dbs.users, NULL, &key, &val, 0);

  /* Check mode - no error message */
  if (!user && ret==DB_NOTFOUND) return ret;

  if (ret == DB_NOTFOUND){
    fprintf(stderr, "Error: no such user: %s\n", name);
    return ret;
  }
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s\n", db_strerror(ret));
    return ret;
  }
  if (user) memcpy(user, val.data, sizeof(user_t));
  return 0;
}

int
user_put(user_t * user, const char * name, int overwrite){
  int ret;
  DBT key = mk_string_dbt(name);
  DBT val;

  memset(&val, 0, sizeof(DBT));
  val.size = sizeof(user_t);
  val.data = user;

  if (check_name(name)!=0) return -1;

  ret = dbs.users->put(dbs.users, NULL, &key, &val,
             overwrite? 0:DB_NOOVERWRITE);

  if (ret == DB_KEYEXIST){
    fprintf(stderr, "Error: user exists: %s\n", name);
    return ret;
  }
  if (ret!=0)
    fprintf(stderr, "Error: can't write user information: %s\n",
      db_strerror(ret));
  return ret;
}


/****************************************************************/

int
auth(const char * name, char * pwd){
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

/****************************************************************/

int
do_level_show(char * user, int level, char **argv){
  printf("%d\n", level);
  return 0;
}

int
do_root_add(char * user, int level, char **argv){
  /* Anybody can add root if it does not exist */
  char *new_pwd=argv[0];
  user_t user_o;

  if ( user_get(NULL, superuser)==0 ){ /* extra check, maybe not needed */
    fprintf(stderr, "Error: superuser exists\n");
    return -1;
  }
  user_o.active = 1;
  user_o.level  = LVL_ROOT;
  if (strlen(new_pwd)<MINPASS){
    fprintf(stderr, "Error: password is too short\n");
    return -1;
  }
  MD5(new_pwd, strlen(new_pwd), user_o.md5);
  return user_put(&user_o, superuser, 0);
}

int
do_user_add(char * user, int level, char **argv){
  char *new_usr=argv[0];
  char *new_pwd=argv[1];
  user_t user_o;

  if (level_check(level, LVL_ADMIN)!=0) return -1;
  user_o.active = 1;
  user_o.level  = LVL_NORM;
  if (strlen(new_pwd)<MINPASS){
    fprintf(stderr, "Error: password is too short\n");
    return -1;
  }
  MD5(new_pwd, strlen(new_pwd), user_o.md5);
  return user_put(&user_o, new_usr, 0);
}

int
do_user_del(char * user, int level, char **argv){
  char *mod_usr = argv[0];
  DBT key = mk_string_dbt(mod_usr);
  int ret;

  if (level_check(level, LVL_ROOT)!=0) return -1;

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't delete superuser\n");
    return -1;
  }

  ret = dbs.users->del(dbs.users, NULL, &key, 0);
  if (ret!=0)
    fprintf(stderr, "Error: can't delete user: %s\n",
      db_strerror(ret));
  return ret;
}

int
do_user_activ_set(char * user, int level, char **argv){
  char *mod_usr = argv[0];
  char *mod_act = argv[1];
  user_t user_o;

  if (level_check(level, LVL_ADMIN)!=0) return -1;
  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't deactivate superuser\n");
    return -1;
  }

  if (user_get(&user_o, mod_usr)!=0) return -1;
  if      (strcmp(mod_act, "on")==0)  user_o.active = 1;
  else if (strcmp(mod_act, "off")==0) user_o.active = 0;
  else {
    fprintf(stderr, "Error: wrong activity seting: %s\n", mod_act);
    return -1;
  }
  return user_put(&user_o, mod_usr, 1);
}

int
do_user_level_set(char * user, int level, char **argv){
  char *mod_usr = argv[0];
  int new_level = get_int(argv[1], "level");
  user_t user_o;

  if (new_level<0)  return -1;

  if (level_check(level, LVL_ADMIN)!=0) return -1;

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't change superuser level\n");
    return -1;
  }

  if (new_level >= LVL_ROOT){
    fprintf(stderr, "Error: level is too high\n");
    return -1;
  }

  if (new_level <  LVL_NORM){
    fprintf(stderr, "Error: wrong level\n");
    return -1;
  }
  if (user_get(&user_o, mod_usr)!=0) return -1;
  user_o.level = new_level;
  return user_put(&user_o, mod_usr, 1);
}

int
do_user_chpwd(char * user, int level, char **argv){
  char *mod_usr = argv[0];
  char *mod_pwd = argv[1];
  user_t user_o;

  if (level_check(level, LVL_ADMIN)!=0) return -1;

  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't change superuser password\n");
    return -1;
  }
  if (user_get(&user_o, mod_usr)!=0) return -1;

  if (strlen(mod_pwd)<MINPASS){
    fprintf(stderr, "Error: password in too short\n");
    return -1;
  }
  MD5(mod_pwd, strlen(mod_pwd), user_o.md5);
  return user_put(&user_o, mod_usr, 1);
}

int
do_user_mypwd(char * user, int level, char **argv){
  const char *pwd = argv[0];
  user_t user_o;

  if (level_check(level, LVL_NORM)!=0) return -1;
  if (user_get(&user_o, user)!=0) return -1;
  if (strlen(pwd)<MINPASS){
    fprintf(stderr, "Error: password in too short\n");
    return -1;
  }
  MD5(pwd, strlen(pwd), user_o.md5);
  return user_put(&user_o, user, 1);
}

/****************************************************************/
/* Print user information.
   modes:
    USR_SHOW_NORM:  name:activity:level
    USR_SHOW_FULL:  all information: name:activity:level:md5 */
#define USR_SHOW_NORM  0
#define USR_SHOW_FULL  100

void
my_show_user(const char * name, user_t *user, int mode){
  int i;
  switch (mode){
    case USR_SHOW_NORM: /* name:activity:level */
      printf("%s:%d:%d", name, user->active, user->level);
      break;
    case USR_SHOW_FULL: /* all information: name:activity:level:md5 */
      printf("%s:%d:%d:", name, user->active, user->level);
      for (i=0; i<sizeof(user->md5); i++) printf("%02X", user->md5[i]);
      break;
  }
  printf("\n");
}

/****************************************************************/

/* Output is different for superuser (md5 is shown)! */
int
do_user_list(char * user, int level, char **argv){
  DBT key = mk_empty_dbt();
  DBT val = mk_empty_dbt();
  DBC *curs;
  int ret;

  ret = dbs.users->cursor(dbs.users, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s \n",
      db_strerror(ret));
    return ret;
  }

  /* Iterate over the database, retrieving each record in turn. */
  while ((ret = curs->get(curs, &key, &val, DB_NEXT)) == 0)
    my_show_user((char *)(key.data), (user_t *)(val.data),
      level==LVL_ROOT? USR_SHOW_FULL:USR_SHOW_NORM);

  if (ret != DB_NOTFOUND)
    fprintf(stderr, "Error: database error: %s\n",
      db_strerror(ret));
  else ret=0;

  if (curs) curs->close(curs);
  return ret;
}

int
do_user_show(char * user, int level, char **argv){
  const char *name = argv[0];
  user_t user_o;
  int ret;

  if ((ret=user_get(&user_o, name))!=0) return ret;
  my_show_user(name, &user_o, USR_SHOW_NORM);
  return ret;
}
