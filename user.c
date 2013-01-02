#include "user.h"
#include <string.h>

/****************************************************************/

#define MAXNAME 30
#define MINPASS 4

int
check_name(const char * name){
  const char accept[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789-_";
  int i, n=strlen(name);
  if (n<1){
    fprintf(stderr, "Error: empty name\n");
    return 1;
  }
  if (n>MAXNAME){
    fprintf(stderr, "Error: too long name (>%d chars)\n", MAXNAME);
    return 1;
  }
  if (strspn(name, accept)!=n){
    fprintf(stderr, "Error: a-z,A-Z,0-9, and _ character only accepted\n");
    return 1;
  }
  return 0;
}

/****************************************************************/

int
user_get(dbs_t *dbs, user_t * user, char * name){
  int ret;
  DBT key = mk_string_dbt(name);
  DBT val = mk_empty_dbt();
  if (strlen(name)<1){
    fprintf(stderr, "Error: empty user name\n");
    return 1;
  }

  ret = dbs->users->get(dbs->users, NULL, &key, &val, 0);

  /* Check mode - no error message */
  if (!user && ret==DB_NOTFOUND) return ret;

  if (ret!=0){
    fprintf(stderr, "Error: can't get user: %s: %s\n",
      name, db_strerror(ret));
    return ret;
  }
  if (user) memcpy(user, val.data, sizeof(user_t));
  return 0;
}

int
user_put(dbs_t *dbs, user_t * user, char * name, int overwrite){
  int ret;
  DBT key = mk_string_dbt(name);
  DBT val;

  memset(&val, 0, sizeof(DBT));
  val.size = sizeof(user_t);
  val.data = user;

  if (check_name(name)!=0) return 1;

  ret = dbs->users->put(dbs->users, NULL, &key, &val,
             overwrite? 0:DB_NOOVERWRITE);
  if (ret!=0)
    fprintf(stderr, "Error: can't write user information: %s\n",
      db_strerror(ret));
  return ret;
}

int
user_del(dbs_t *dbs, char * name){
  int ret;
  DBT key = mk_string_dbt(name);
  ret = dbs->users->del(dbs->users, NULL, &key, 0);
  if (ret!=0)
    fprintf(stderr, "Error: can't delete user: %s\n",
      db_strerror(ret));
  return ret;
}

/****************************************************************/

int
user_check(dbs_t *dbs, char * name, char *pwd, int level){
  int ret;
  user_t user;
  if (level<0) return 0;

  if (strlen(name)!=0 &&
      user_get(dbs, NULL, name)==0 && // w/o error message
      user_get(dbs, &user, name)==0 &&
      user.active && user.level>=level &&
      memcmp(MD5(pwd, strlen(pwd),NULL),
                user.md5, sizeof(user.md5))==0) return 0;

//  sleep(1);
  fprintf(stderr, "Error: wrong user/password\n");
  return 1;
}

int
user_add(dbs_t *dbs, char * name, char *pwd, int level){
  user_t user;
  user.active = 1;
  user.level  = level;
  if (strlen(pwd)<MINPASS){
    fprintf(stderr, "Error: password in too short\n");
    return 1;
  }
  MD5(pwd, strlen(pwd), user.md5);
  return user_put(dbs, &user, name, 0);
}

int
user_chpwd(dbs_t *dbs, char * name, char *pwd){
  int ret;
  user_t user;
  ret = user_get(dbs, &user, name);
  if (ret) return ret;
  if (strlen(pwd)<MINPASS){
    fprintf(stderr, "Error: password in too short\n");
    return 1;
  }
  MD5(pwd, strlen(pwd), user.md5);
  return user_put(dbs, &user, name, 1);
}

int
user_chact(dbs_t *dbs, char * name, int act){
  int ret;
  user_t user;
  ret = user_get(dbs, &user, name);
  if (ret) return ret;
  user.active = act;
  return user_put(dbs, &user, name, 1);
}

int
user_chlvl(dbs_t *dbs, char * name, int level){
  int ret;
  user_t user;
  ret = user_get(dbs, &user, name);
  if (ret) return ret;
  user.level = level;
  return user_put(dbs, &user, name, 1);
}

void
my_show_user(char * name, user_t *user, int mode){
  int i;
  switch (mode){
    case USR_SHOW_NORM: /* name:activity:level */
      printf("%s:%d:%d", name, user->active, user->level);
      break;
    case USR_SHOW_NAME:  /* list only active users */
      if (user->active) printf("%s", name);
      break;
    case USR_SHOW_LEVEL:  /* user level */
      printf("%d", user->active? user->level:-1);
      break;
    case USR_SHOW_FULL: /* all information: name:activity:level:md5 */
      printf("%s:%d:%d:", name, user->active, user->level);
      for (i=0; i<sizeof(user->md5); i++) printf("%02X", user->md5[i]);
      break;
  }
  printf("\n");
}

int
user_list(dbs_t *dbs, int mode){
  int ret;
  DBT key = mk_empty_dbt();
  DBT val = mk_empty_dbt();
  DBC *curs;

  dbs->users->cursor(dbs->users, NULL, &curs, 0);

  /* Iterate over the database, retrieving each record in turn. */
  while ((ret = curs->get(curs, &key, &val, DB_NEXT)) == 0)
    my_show_user((char *)(key.data), (user_t *)(val.data), mode);

  if (ret != DB_NOTFOUND)
    fprintf(stderr, "Error: can't get user list: %s\n",
      db_strerror(ret));
  else ret=0;

  if (curs) curs->close(curs);
  return ret;
}

int
user_show(dbs_t *dbs, char *name, int mode){
  int ret;
  user_t user;
  if ((ret=user_get(dbs, &user, name))!=0) return ret;
  my_show_user(name, &user, mode);
  return ret;
}
