#include "actions.h"
#include <string.h>
#include <time.h>

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

