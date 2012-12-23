#include "actions.h"
#include "user.h"

char * superuser = "root";
char * user_edit_group = "user_edit";

/*********************************************************************/

int
do_user_check(dbs_t * dbs, char * user, int argc, char **argv){
  return 0; /* auth check is performed outside */
}

int
do_root_add(dbs_t * dbs, char * user, int argc, char **argv){
  char *new_pwd=argv[0];
  if ( user_get(dbs, NULL, superuser)==0 ){ /* extra check, maybe not needed */
    fprintf(stderr, "Error: superuser exists\n");
    return 1;
  }
  return user_add(dbs, superuser, new_pwd);
}

int
do_user_add(dbs_t * dbs, char * user, int argc, char **argv){
  char *new_usr=argv[0];
  char *new_pwd=argv[1];

  /* only root or users from user_edit_group */
  if (strcmp(user, superuser)!=0 &&
      group_check(dbs, user, user_edit_group)!=0) return 1;

  return user_add(dbs, new_usr, new_pwd);
}

int
do_user_del(dbs_t * dbs, char * user, int argc, char **argv){
  char *del_usr = argv[0];

  /* only root */
  if (strcmp(user, superuser)!=0) return 1;

  return user_del(dbs, del_usr);
}

int
do_user_on(dbs_t * dbs, char * user, int argc, char **argv){
  char *mod_usr = argv[0];

  /* only root or users from user_edit_group */
  if (strcmp(user, superuser)!=0 &&
      group_check(dbs, user, user_edit_group)!=0) return 1;

  return user_chact(dbs, mod_usr, 1);
}

int
do_user_off(dbs_t * dbs, char * user, int argc, char **argv){
  char *mod_usr = argv[0];

  /* root must be active */
  if (strcmp(mod_usr, superuser)==0){
    fprintf(stderr, "Error: can't deactivate superuser\n");
    return 1;
  }

  /* only root or users from user_edit_group */
  if (strcmp(user, superuser)!=0 &&
      group_check(dbs, user, user_edit_group)!=0) return 1;

  return user_chact(dbs, mod_usr, 0);
}

int
do_user_chpwd(dbs_t * dbs, char * user, int argc, char **argv){
  char *mod_usr = argv[0];
  char *mod_pwd = argv[1];

  /* Only user itself and root */
  if (strcmp(user, superuser)!=0 &&
      strcmp(user, mod_usr)!=0){
    fprintf(stderr, "Error: permission denied.\n");
    return 1;
  }

  return user_chpwd(dbs, mod_usr, mod_pwd);
}


int
do_user_list(dbs_t * dbs, char * user, int argc, char **argv){
  return user_list(dbs, 1);
}

int
do_user_dump(dbs_t * dbs, char * user, int argc, char **argv){

  /* only root */
  if (strcmp(user, superuser)!=0) return 1;

  return user_list(dbs, 2);
}

int
do_user_show(dbs_t * dbs, char * user, int argc, char **argv){
  char *name = argv[0];
  return user_show(dbs, name, 1);
}

/*********************************************************************/

int
do_group_add(dbs_t * dbs, char * user, int argc, char **argv){
  char *mod_usr = argv[0];
  char *group   = argv[1];

  /* only root or users from user_edit_group */
  if (strcmp(user, superuser)!=0 &&
      group_check(dbs, user, user_edit_group)!=0) return 1;

  return group_add(dbs, mod_usr, group);
}

int
do_group_del(dbs_t * dbs, char * user, int argc, char **argv){
  char *mod_usr = argv[0];
  char *group   = argv[1];

  /* only root or users from user_edit_group */
  if (strcmp(user, superuser)!=0 &&
      group_check(dbs, user, user_edit_group)!=0) return 1;

  return group_del(dbs, mod_usr, group);
}

int
do_group_check(dbs_t * dbs, char * user, int argc, char **argv){
  char *group = argv[0];
  return group_check(dbs, user, group);
}

int
do_group_list(dbs_t * dbs, char * user, int argc, char **argv){
  char *mod_usr = argv[0];
  return group_list(dbs, mod_usr,'\n');
}
