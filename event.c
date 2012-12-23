#include "event.h"
#include "string.h"
#include "stdlib.h"



/*********************************************************************/
/* Replace '<' and '>' by '[' and ']' in the string.
   Replace '\n' by ' ' if rem_nl!=0 */
# define REMOVE_NL 0
# define KEEP_NL   1
void
remove_html(char * str, int rem_nl){
  int i;
  for (i=0; i<strlen(str); i++){
    if (str[i]=='<') str[i]='[';
    if (str[i]=='>') str[i]=']';
    if (rem_nl && str[i]=='\n') str[i]=' ';
  }
}

/*********************************************************************/

/* Build DBT object from event structure.
   Html <tags> is converted to [tags], '\n' is converted to ' '
   in all fields but 'body'.
   Space for .data is allocated, this field must be freed after use!
   If error occures val.data is set to NULL! */
DBT
event2dbt(event_t * event){
  DBT val = mk_empty_dbt();
  int ptr, i;
  event_t * ev;

  if (event->date1 < 0) event->date1=0;
  if (event->date2 < event->date1) event->date2=event->date1;

  /* calculate data size and build data structure */
  val.size = sizeof(event_t)   /* static fields + pointers */
           + strlen(event->title) + 1 /* including \0*/
           + strlen(event->body) + 1
           + strlen(event->people) + 1
           + strlen(event->route) + 1
           + sizeof(int) * event->ntags;
  val.data = malloc(val.size);
  if (val.data==NULL){
    fprintf(stderr, "Error: can't allocate memory\n");
    return val;
  }
  /* copy event to the header of val.data */
  ev = (event_t *)val.data;
  *ev = *event;

  ptr=sizeof(event_t);
  strcpy(val.data + ptr, event->title); /* copy data */
    remove_html(val.data + ptr,  REMOVE_NL);
    ev->title = NULL + ptr;
    ptr += strlen(event->title) + 1;
  strcpy(val.data + ptr, event->body);
    remove_html(val.data + ptr,  KEEP_NL);
    ev->body = NULL + ptr;
    ptr += strlen(event->body) + 1;
  strcpy(val.data + ptr, event->people);
    remove_html(val.data + ptr,  REMOVE_NL);
    ev->people = NULL + ptr;
    ptr += strlen(event->people) + 1;
  strcpy(val.data + ptr, event->route);
    remove_html(val.data + ptr,  REMOVE_NL);
    ev->route = NULL + ptr;
    ptr += strlen(event->route) + 1;

  ev->tags = (int *)(NULL + ptr);
  for (i=0; i<event->ntags; i++){
    * (int *)(val.data + ptr) = event->tags[i];
    ptr+=sizeof(int);
  }
  return val;
}

/* Convert DBT structure to event. Data is not copied! */
event_t
dbt2event(DBT * dbt){
  event_t ev = * (event_t *)dbt->data;
  /* Overwrite pointers to absolute values */
  ev.title  = (char *)dbt->data + ((void*)ev.title - NULL); /* (char*) + (int)(void*-void*) */
  ev.body   = (char *)dbt->data + ((void*)ev.body - NULL);
  ev.people = (char *)dbt->data + ((void*)ev.people - NULL);
  ev.route  = (char *)dbt->data + ((void*)ev.route - NULL);
  ev.tags   = (int *)(dbt->data + ((void*)ev.tags - NULL)); /* careful with types!*/
  return ev;
}

/* print event to stdout */
void
print_event(int id, event_t * ev){
  int i;
  printf("<event id=%d date1=%d date2=%d>\n", id, ev->date1, ev->date2);
  printf("<title>%s</title>\n",  ev->title);
  printf("<body>%s</body>\n",    ev->body);
  printf("<people>%s</people>\n",ev->people);
  printf("<route>%s</route>\n",  ev->route);
  printf("<tags>");
  for (i=0; i<ev->ntags; i++) printf("%s%d", i==0?"":",", ev->tags[i]);
  printf("</tags>\n");
  printf("</event>\n");
}

/*********************************************************************/

int
event_last_id(dbs_t *dbs){
  int id, ret;
  DBC *curs;
  DBT key = mk_empty_dbt();
  DBT val = mk_empty_dbt();

  ret = dbs->events->cursor(dbs->events, NULL, &curs, 0)==0? 0:-1;

  if (ret==0){
    ret = curs->get(curs, &key, &val, DB_PREV);
    switch (ret){
      case 0: id = *(int *)key.data; break;
      case DB_NOTFOUND: id=ret=0; break; /* no entries */
    }
    if (curs != NULL) curs->close(curs);
  }

  if (ret!=0){
    fprintf(stderr, "Error: can't get id from database: %s \n",
      db_strerror(ret));
    id=-1;
  }
  return id;
}

int
event_put(dbs_t *dbs, int id, event_t * event, int overwrite){
  int ret;
  DBT key = mk_int_dbt(&id);
  DBT val = event2dbt(event);

  if (val.data==NULL) return 1;

  if (id<1){
    fprintf(stderr, "Error: wrong event id: %d\n", id);
    return 1;
  }

  ret = dbs->events->put(dbs->events, NULL, &key, &val,
             overwrite? 0:DB_NOOVERWRITE);
  if (ret!=0)
    fprintf(stderr, "Error: can't write user information: %s\n",
      db_strerror(ret));

  free(val.data);
  return ret;
}

int
event_new(dbs_t *dbs, event_t * event){
  int id,ret;

  if ((id = event_last_id(dbs))<0) return -1;
  id++;

  ret = event_put(dbs, id, event, 1);
  if (ret == 0) printf("%d\n", id);
  return ret;
}

int
event_del(dbs_t *dbs, int id){
  int ret;
  DBT key = mk_int_dbt(&id);

  ret = dbs->events->del(dbs->events, NULL, &key, 0);
  if (ret!=0)
    fprintf(stderr, "Error: can't delete event: %s\n",
      db_strerror(ret));
  return ret;
}

int
event_print(dbs_t *dbs, int id){
  int ret;
  DBT key = mk_int_dbt(&id);
  DBT val = mk_empty_dbt();
  event_t ev;

  ret = dbs->events->get(dbs->events, NULL, &key, &val, 0);
  if (ret!=0){
    fprintf(stderr, "Error: can't get event %d: %s\n",
      id, db_strerror(ret));
    return ret;
  }
  ev = dbt2event(&val);
  print_event(id, &ev);
  return 0;
}

int
event_search(dbs_t *dbs, event_t * mask){
  int ret, i, j;
  unsigned int d1,d2;
  DBC *curs;
  DBT key = mk_empty_dbt();
  DBT val = mk_empty_dbt();
  event_t ev;

  ret = dbs->events->cursor(dbs->events, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: can't get id from database: %s \n",
      db_strerror(ret));
    return ret;
  }

  d1 = (mask->date1 >= 0)?  mask->date1:0;
  d2 = (mask->date2 >= 0)?  mask->date2:~0;
  if (d2<d1) d2=d1;

  while ((ret = curs->get(curs, &key, &val, DB_NEXT))==0){
    ev = dbt2event(&val);
    /* search by date range: */
    if (d1 && d1 > ev.date2) continue;
    if (d2 && d2 < ev.date1) continue;

    /* search by tags: */
    if (mask->ntags > 0){
      int fl=0;
      for (i=0; i<mask->ntags; i++){
        for (j=0; j<ev.ntags; j++){
          if (mask->tags[i] == ev.tags[j]) fl=1;
        }
      }
      if (fl==0) continue;
    }

    /* search in text fields: */
    if (strlen(mask->title)  && !strcasestr(ev.title,  mask->title))  continue;
    if (strlen(mask->body)   && !strcasestr(ev.body,   mask->body))   continue;
    if (strlen(mask->people) && !strcasestr(ev.people, mask->people)) continue;
    if (strlen(mask->route)  && !strcasestr(ev.route,  mask->route))  continue;

    print_event(* (int*)key.data, &ev);
  }

  if (curs != NULL) curs->close(curs);

  if (ret!=DB_NOTFOUND){
    fprintf(stderr, "Error: can't get id from database: %s \n",
      db_strerror(ret));
    return ret;
  }
  return 0;
}
