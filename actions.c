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
  if (strlen(str)==1 && str[0]=='0') return 0;
  ret=atoi(str);
  if (ret<=0){
    fprintf(stderr, "Error: bad %s: %s\n", name, str);
    return -1;
  }
  return ret;
}

int
get_tags(char *str, int tags[MAX_TAGS]){
  char *stag, *prev;
  int i = 0;
  stag = str;

  if (tags == NULL){
    fprintf(stderr, "Error: bad tag memory\n");
    return -1;
  }
  while (stag && (prev = strsep(&stag, ",:; \n\t"))){
    if (i>MAX_TAGS-1){
      fprintf(stderr, "Too many tags (> %d)\n", MAX_TAGS-1);
      return -1;
    }
    if (strlen(prev)){
      tags[i] = atoi(prev);
      if (tags[i]==0){
        fprintf(stderr, "Error: bad tags: %s\n", str);
        return -1;
      }
    }
    i++;
  }
  return i;
}
