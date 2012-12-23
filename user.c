#include "user.h"
#include <string.h>

/****************************************************************/

#define MAXNAME 30

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
user_check(dbs_t *dbs, char * name, char *pwd){
  int ret;
  user_t user;
  if (strlen(name)!=0 &&
      user_get(dbs, NULL, name)==0 && // w/o error message
      user_get(dbs, &user, name)==0 &&
      user.active &&
      memcmp(MD5(pwd, strlen(pwd),NULL),
                user.md5, sizeof(user.md5))==0) return 0;

  fprintf(stderr, "Error: wrong user/password\n");
  return 1;
}

int
user_add(dbs_t *dbs, char * name, char *pwd){
  user_t user;
  user.active=1;
  MD5(pwd, strlen(pwd), user.md5);
  return user_put(dbs, &user, name, 0);
}

int
user_chpwd(dbs_t *dbs, char * name, char *pwd){
  int ret;
  user_t user;
  ret = user_get(dbs, &user, name);
  if (ret) return ret;
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
user_list(dbs_t *dbs, int mode){

  int ret;
  DBT key = mk_empty_dbt();
  DBT val = mk_empty_dbt();
  DBC *curs;

  dbs->users->cursor(dbs->users, NULL, &curs, 0);

  /* Iterate over the database, retrieving each record in turn. */
  while ((ret = curs->get(curs, &key, &val, DB_NEXT)) == 0) {
    int i;
    user_t *user=(user_t *)(val.data);

    switch (mode){
      case 0:  /* list only active users */
        printf("%s", (char *)(key.data));
        break;
      case 1: /* list names, activity, groups */
        printf("%s:%d:", (char *)(key.data), user->active);
        group_list(dbs, (char *)(key.data), ' ');
        break;
      case 2: /* list full information (with md5) */
        printf("%s:%d:", (char *)(key.data), user->active);
        for (i=0; i<sizeof(user->md5); i++) printf("%02X", user->md5[i]);
        printf(":");
        group_list(dbs, (char *)(key.data), ' ');
        break;
    }
    printf("\n");
  }
  if (ret != DB_NOTFOUND)
    fprintf(stderr, "Error: can't get user list: %s\n",
      db_strerror(ret));
  else ret=0;

  if (curs) curs->close(curs);
  return ret;
}

int
user_show(dbs_t *dbs, char *name, int mode){
  int ret, i;
  user_t user;
  if ((ret=user_get(dbs, &user, name))!=0) return ret;

  switch (mode){
    case 0:  /* list only active users */
      printf("%s", name);
      break;
    case 1: /* list names, activity, groups */
      printf("%s:%d:", name, user.active);
      ret=group_list(dbs, name, ' ');
      break;
    case 2: /* list full information (with md5) */
      printf("%s:%d:", name, user.active);
      for (i=0; i<sizeof(user.md5); i++) printf("%02X", user.md5[i]);
      printf(":");
      ret=group_list(dbs, name, ' ');
      break;
  }
  printf("\n");
  return ret;
}

/***********************************************************/

int group_add(dbs_t *dbs, char * user, char * group){
  int ret;
  DBT key = mk_string_dbt(user);
  DBT val = mk_string_dbt(group);

  if (check_name(group)!=0) return 1;

  if (user_get(dbs, NULL, user)!=0) return 1;

  ret = dbs->groups->put(dbs->groups, NULL,
    &key, &val, DB_NODUPDATA);

  if (ret!=0)
    fprintf(stderr, "Error: can't update group information: %s\n",
      db_strerror(ret));
  return ret;
}

int group_del(dbs_t *dbs, char * user, char * group){
  int ret;
  DBT key = mk_string_dbt(user);
  DBT val = mk_string_dbt(group);
  DBC *curs;

  dbs->groups->cursor(dbs->groups, NULL, &curs, 0);

  ret=curs->get(curs, &key, &val, DB_GET_BOTH);
  if (ret==0) curs->del(curs, 0);
  else fprintf(stderr, "Error: can't remove user from group: %s\n",
         db_strerror(ret));

  if (curs) curs->close(curs);
  return ret;
}

int group_check(dbs_t *dbs, char * user, char * group){
  int ret;
  DBT key = mk_string_dbt(user);
  DBT val = mk_string_dbt(group);
  DBC *curs;
  dbs->groups->cursor(dbs->groups, NULL, &curs, 0);
  ret = curs->get(curs, &key, &val, DB_GET_BOTH);

  if (ret!=0 && ret != DB_NOTFOUND)
    fprintf(stderr, "Error: can't check group: %s\n",
      db_strerror(ret));

  if (ret == DB_NOTFOUND)
    fprintf(stderr, "Error: user is not in the group: %s\n",
      group);

  if (curs) curs->close(curs);
  return ret;
}

int group_list(dbs_t *dbs, char * user, char sep){
  int ret;
  DBT key = mk_string_dbt(user);
  DBT val = mk_empty_dbt();
  DBC *curs;

  dbs->groups->cursor(dbs->groups, NULL, &curs, 0);

  ret=curs->get(curs, &key, &val, DB_SET);
  while (ret == 0){
    printf("%s%c", (char *)val.data, sep);
    ret=curs->get(curs, &key, &val, DB_NEXT_DUP);
  }

  if (ret != DB_NOTFOUND)
    fprintf(stderr, "Error: can't get group list: %s\n",
      db_strerror(ret));
  else ret=0;

  if (curs) curs->close(curs);
  return ret;
}
