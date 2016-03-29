#include "actions.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"
#include "event.h"

/*********************************************************************/
/* structure for log database */
typedef struct {
  int32_t event;
  char_ptr user,
           action,
           msg;
} log_t;

/*********************************************************************/
/* Build DBT object from event structure.
   Html <tags> is converted to [tags], '\n' is converted to ' '
   in all fields but 'body'.
   Space for .data is allocated, this field must be freed after use!
   If error occures val.data is set to NULL! */
DBT
log2dbt(log_t * log){

  DBT val = mk_empty_dbt();
  int ptr, i;
  log_t * l1;

  if (log->event < 0) log->event=0;

  /* calculate data size and build data structure */
  val.size = sizeof(log_t)   /* static fields + pointers */
           + strlen(log->user.ptr) + 1 /* including \0*/
           + strlen(log->action.ptr) + 1
           + strlen(log->msg.ptr) + 1;
  val.data = malloc(val.size);
  if (val.data==NULL){
    fprintf(stderr, "Error: can't allocate memory\n");
    return val;
  }
  /* copy log to the header of val.data */
  l1 = (log_t *)val.data;
  *l1 = *log;

  ptr=sizeof(log_t);
  strcpy(val.data + ptr, log->user.ptr); /* copy data */
    remove_html(val.data + ptr,  REMOVE_NL);
    l1->user.off = ptr;
    ptr += strlen(log->user.ptr) + 1;
  strcpy(val.data + ptr, log->action.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    l1->action.off = ptr;
    ptr += strlen(log->action.ptr) + 1;
  strcpy(val.data + ptr, log->msg.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    l1->msg.off = ptr;
    ptr += strlen(log->msg.ptr) + 1;
  return val;
}

/* Convert DBT structure to log. Data is not copied! */
log_t
dbt2log(const DBT * dbt){
  log_t l1 = * (log_t *)dbt->data;
  /* Overwrite pointers to absolute values */
  l1.user.ptr   = (char *)dbt->data + l1.user.off; /* (char*) + (int)(void*-void*) */
  l1.action.ptr = (char *)dbt->data + l1.action.off;
  l1.msg.ptr    = (char *)dbt->data + l1.msg.off;
  return l1;
}

/* print log to stdout */
void
print_log(int id, log_t * l1){
  int i;
  printf("<log id=%d event=%d>\n", id, l1->event);
  printf(" <user>%s</user>\n",     l1->user.ptr);
  printf(" <action>%s</action>\n", l1->action.ptr);
  printf(" <msg>%s</msg>\n",       l1->msg.ptr);
  printf("</log>\n");
}

/*********************************************************************/

int
do_log_new(char * user, int level, char **argv){
  unsigned int id = time(NULL);
  int ret;
  log_t log;
  DBT key, val;

  log.event  = atoi(argv[0]);
  log.user.ptr   = argv[1];
  log.action.ptr = argv[2];
  log.msg.ptr    = argv[3];

  key = mk_uint_dbt(&id);
  val = log2dbt(&log); /* do free after use!*/
  if (val.data==NULL) return -1;

  /* Try to put log, if id exists, increase it: */
  do {
    ret = dbs.logs->put(dbs.logs, NULL, &key, &val, DB_NOOVERWRITE);
    id++;
  } while (ret==DB_KEYEXIST);

  if (ret!=0)
    fprintf(stderr, "Error: can't write log: %s\n",
      db_strerror(ret));
  else printf("%d\n", id-1);

  free(val.data);
  return ret;
}

int
do_log_print(char * user, int level, char **argv){
  int ret;
  unsigned int id = atoi(argv[0]);
  DBT key = mk_uint_dbt(&id);
  DBT val = mk_empty_dbt();
  log_t l1;

  ret = dbs.logs->get(dbs.logs, NULL, &key, &val, 0);
  if (ret!=0){
    fprintf(stderr, "Error: can't get log %d: %s\n",
      id, db_strerror(ret));
    return ret;
  }
  l1 = dbt2log(&val);
  print_log(id, &l1);
  return 0;
}

int
do_log_tsearch(char * user, int level, char **argv){
  int ret;
  unsigned int t1,t2;
  DBC *curs;
  DBT key,val;

  t1=t2=time(NULL);
  if (strlen(argv[0])) t1=atoi(argv[0]);
  if (strlen(argv[1])) t2=atoi(argv[1]);
  if (t2<t1) t2=t1;

  key = mk_uint_dbt(&t1);
  val = mk_empty_dbt();

  ret = dbs.logs->cursor(dbs.logs, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s \n",
      db_strerror(ret));
    return ret;
  }

  ret = curs->get(curs, &key, &val, DB_SET_RANGE);
  while (ret==0 && *(int *)key.data <= t2){
    log_t l1 = dbt2log(&val);
    print_log(*(int *)key.data, &l1);
    ret = curs->get(curs, &key, &val, DB_NEXT);
  }

  if (curs != NULL) curs->close(curs);

  if (ret!=0 && ret!=DB_NOTFOUND){
    fprintf(stderr, "Error: can't get id from database: %s \n",
      db_strerror(ret));
    return ret;
  }
  return 0;
}

int
do_log_esearch(char * user, int level, char **argv){
  /*TODO*/
}
