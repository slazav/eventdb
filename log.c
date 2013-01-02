#include "log.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

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
           + strlen(log->user) + 1 /* including \0*/
           + strlen(log->action) + 1
           + strlen(log->msg) + 1;
  val.data = malloc(val.size);
  if (val.data==NULL){
    fprintf(stderr, "Error: can't allocate memory\n");
    return val;
  }
  /* copy log to the header of val.data */
  l1 = (log_t *)val.data;
  *l1 = *log;

  ptr=sizeof(log_t);
  strcpy(val.data + ptr, log->user); /* copy data */
    remove_html(val.data + ptr,  REMOVE_NL);
    l1->user = NULL + ptr;
    ptr += strlen(log->user) + 1;
  strcpy(val.data + ptr, log->action);
    remove_html(val.data + ptr,  REMOVE_NL);
    l1->action = NULL + ptr;
    ptr += strlen(log->action) + 1;
  strcpy(val.data + ptr, log->msg);
    remove_html(val.data + ptr,  REMOVE_NL);
    l1->msg = NULL + ptr;
    ptr += strlen(log->msg) + 1;
  return val;
}

/* Convert DBT structure to log. Data is not copied! */
log_t
dbt2log(DBT * dbt){
  log_t l1 = * (log_t *)dbt->data;
  /* Overwrite pointers to absolute values */
  l1.user   = (char *)dbt->data + ((void*)l1.user - NULL); /* (char*) + (int)(void*-void*) */
  l1.action = (char *)dbt->data + ((void*)l1.action - NULL);
  l1.msg    = (char *)dbt->data + ((void*)l1.msg - NULL);
  return l1;
}

/* print log to stdout */
void
print_log(int id, log_t * l1){
  int i;
  printf("<log id=%d event=%d>\n", id, l1->event);
  printf(" <user>%s</user>\n",     l1->user);
  printf(" <action>%s</action>\n", l1->action);
  printf(" <msg>%s</msg>\n",       l1->msg);
  printf("</log>\n");
}

/*********************************************************************/

int
log_new(log_t * log){
  int ret;
  unsigned int id = time(NULL);
  DBT key = mk_uint_dbt(&id);
  DBT val = log2dbt(log);

  if (val.data==NULL) return 1;

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
log_print(unsigned int id){
  int ret;
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
log_tsearch(unsigned int t1, unsigned int t2){
  int ret;
  DBC *curs;
  DBT key = mk_uint_dbt(&t1);
  DBT val = mk_empty_dbt();

  ret = dbs.logs->cursor(dbs.logs, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: can't get log entry: %s \n",
      db_strerror(ret));
    return ret;
  }

  if (t2<t1) t2=t1;

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
