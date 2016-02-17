#include "event.h"
#include <string.h>
#include <stdlib.h>

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
           + strlen(event->title.ptr) + 1 /* including \0*/
           + strlen(event->body.ptr) + 1
           + strlen(event->people.ptr) + 1
           + strlen(event->route.ptr) + 1
           + strlen(event->owner.ptr) + 1
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
  strcpy(val.data + ptr, event->title.ptr); /* copy data */
    remove_html(val.data + ptr,  REMOVE_NL);
    ev->title.off = ptr;
    ptr += strlen(event->title.ptr) + 1;
  strcpy(val.data + ptr, event->body.ptr);
    remove_html(val.data + ptr,  KEEP_NL);
    ev->body.off = ptr;
    ptr += strlen(event->body.ptr) + 1;
  strcpy(val.data + ptr, event->people.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    ev->people.off = ptr;
    ptr += strlen(event->people.ptr) + 1;
  strcpy(val.data + ptr, event->route.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    ev->route.off = ptr;
    ptr += strlen(event->route.ptr) + 1;
  strcpy(val.data + ptr, event->owner.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    ev->owner.off = ptr;
    ptr += strlen(event->owner.ptr) + 1;

  ev->tags.off = ptr;
  for (i=0; i<event->ntags; i++){
    * (int32_t *)(val.data + ptr) = event->tags.ptr[i];
    ptr+=sizeof(int);
  }
  return val;
}

/* Convert DBT structure to event. Data is not copied! */
event_t
dbt2event(const DBT * dbt){
  event_t ev = * (event_t *)dbt->data;
  /* Overwrite pointers to absolute values */
  ev.title.ptr  = (char *)dbt->data + ev.title.off; /* (char*) + (int) */
  ev.body.ptr   = (char *)dbt->data + ev.body.off;
  ev.people.ptr = (char *)dbt->data + ev.people.off;
  ev.route.ptr  = (char *)dbt->data + ev.route.off;
  ev.owner.ptr  = (char *)dbt->data + ev.owner.off;
  ev.tags.ptr   = (uint32_t *)(dbt->data + ev.tags.off);
  return ev;
}

/* date1 extractor for the secondary db */
int
db_event_date1(DB *secdb, const DBT *pkey, const DBT *pdata, DBT *skey){
  static int val;
  event_t ev = dbt2event(pdata);
  memset(skey, 0, sizeof(DBT));
  val=ev.date1;
  skey->data = &val;
  skey->size = sizeof(int);
  return 0;
}

/* print event to stdout */
void
event_prn(int id, event_t * ev){
  int i;
  printf("<event id=%d>\n", id);
  printf(" <ctime>%d</ctime>\n",   ev->ctime);
  printf(" <mtime>%d</mtime>\n",   ev->mtime);
  printf(" <date1>%d</date1>\n",   ev->date1);
  printf(" <date2>%d</date2>\n",   ev->date2);
  printf(" <title>%s</title>\n",  ev->title.ptr);
  printf(" <body>%s</body>\n",    ev->body.ptr);
  printf(" <people>%s</people>\n",ev->people.ptr);
  printf(" <route>%s</route>\n",  ev->route.ptr);
  printf(" <owner>%s</owner>\n",  ev->owner.ptr);
  printf(" <tags>");
  for (i=0; i<ev->ntags; i++) printf("%s%d", i==0?"":",", ev->tags.ptr[i]);
  printf("</tags>\n");
  list_event_links(id);
  printf("</event>\n");
}

/* get last event id in the database */
int
event_last_id(){
  int id, ret;
  DBC *curs;
  DBT key = mk_empty_dbt();
  DBT val = mk_empty_dbt();

  ret = dbs.events->cursor(dbs.events, NULL, &curs, 0)==0? 0:-1;

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

/* Get event information from argv[0..6] and put it to event and tags. */
/* NOTE: event.tags must be a valid pointer to int[MAX_TAGS]. */
int
event_parse(char **argv, event_t * event){
  event->title.ptr  = argv[0];
  event->body.ptr   = argv[1];
  event->people.ptr = argv[2];
  event->route.ptr  = argv[3];
  event->date1 = get_int(argv[4], "date1"); if (event->date1<0) return -1;
  event->date2 = get_int(argv[5], "date2"); if (event->date2<0) return -1;
  if (event->date2<event->date1) event->date2=event->date1;
  if ((event->ntags=get_tags(argv[6], event->tags.ptr)) <0) return -1;
  return 0;
}


int
event_put(int id, event_t *ev, int flags){
  DBT key = mk_uint_dbt(&id);
  DBT val = event2dbt(ev); /* do free before return! */
  int ret;

  if (val.data==NULL) return -1;

  /* write event */
  ret = dbs.events->put(dbs.events, NULL, &key, &val, flags);
  if (ret!=0)
    fprintf(stderr, "Error: can't write event: %s\n",
      db_strerror(ret));
  free(val.data);
  return ret;
}

int
event_get(int id, event_t *ev){
  DBT key = mk_uint_dbt(&id);
  DBT val = mk_empty_dbt();
  int ret = dbs.events->get(dbs.events, NULL, &key, &val, 0);
  if (ret==DB_NOTFOUND){
    fprintf(stderr, "Error: event not found: %d \n", id);
    return ret;
  }
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s\n", db_strerror(ret));
    return ret;
  }
  *ev = dbt2event(&val);
  return 0;
}

/* Check permissions for data modifications:
  ANON - no
  NOAUTH and NORM - owner only,
  MODER and higher - yes */
int
event_mperm_check(int id, char *user, int level){
  int ret;
  event_t obj;

  if (level_check(level, LVL_NOAUTH)!=0) return -1;
  if (level >= LVL_MODER) return 0;

  if (event_get(id, &obj)!=0) return -1;
  if (strcmp(user, obj.owner.ptr)==0) return 0;

  fprintf(stderr, "Error: %s is not allowed to modify data owned by %s\n",
      user, obj.owner.ptr);
  return -1;
}

/*********************************************************************/
/* Actions */
int
do_event_create(char * user, int level, char **argv){
  int id;
  int tags[MAX_TAGS];
  event_t ev;

  /* Check permissions: only ANON can't create events */
  if (level_check(level, LVL_NOAUTH)!=0) return -1;

  /* Find last existing event id */
  if ((id = event_last_id(dbs))<0) return -1;
  id++;

  /* Parse arguments and make DBT for new event */
  ev.ctime = time(NULL);
  ev.mtime = ev.ctime;
  ev.owner.ptr = user;
  ev.tags.ptr  = tags;
  if (event_parse(argv, &ev)!=0) return -1;

  if (event_put(id, &ev, DB_NOOVERWRITE)!=0) return -1;
  printf("%d\n", id);
  return 0;
}

int
do_event_edit(char * user, int level, char **argv){
  int id;
  int tags[MAX_TAGS];
  event_t ev, oev;

  /* get id from cmdline */
  id = get_int(argv[0], "event id");
  if (id <= 0)  return -1;

  /* Check permissions */
  if (event_mperm_check(id, user, level)!=0) return -1;

  if (event_get(id, &oev)!=0) return -1;

  /* Parse arguments and make DBT for new event */
  ev.tags.ptr = tags;
  if (event_parse(argv+1, &ev)!=0) return -1;
  ev.mtime = time(NULL);
  ev.ctime = oev.ctime;
  ev.owner = oev.owner;

  return event_put(id, &ev, 0);
}


int
do_event_delete(char * user, int level, char **argv){
  int id;
  int ret;
  DBT key = mk_uint_dbt(&id);

  /* get id from cmdline */
  id = get_int(argv[0], "event id");
  if (id <= 0)  return -1;

  /* Check permissions */
  if (event_mperm_check(id, user, level)!=0) return -1;

  /* delete event */
  ret = dbs.events->del(dbs.events, NULL, &key, 0);
  if (ret==DB_NOTFOUND){
    fprintf(stderr, "Error: event not found: %d \n", id);
    return ret;
  }
  else if (ret!=0){
    fprintf(stderr, "Error: database error: %s\n", db_strerror(ret));
    return ret;
  }
  return ret;
}

int
do_event_show(char * user, int level, char **argv){
  int id;
  event_t ev;

  /* get id from cmdline */
  id = get_int(argv[0], "event id");
  if (id == 0)  return -1;

  if (event_get(id, &ev)!=0) return -1;
  event_prn(id, &ev);
  return 0;
}

int
do_event_list(char * user, int level, char **argv){
  DBC *curs;
  DBT key  = mk_empty_dbt();
  DBT pkey = mk_empty_dbt();
  DBT pval  = mk_empty_dbt();
  event_t ev;
  int ret;

  ret = dbs.events->cursor(dbs.d2ev, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }

  while ((ret = curs->pget(curs, &key, &pkey, &pval, DB_PREV))==0){
    ev = dbt2event(&pval);
    event_prn(* (int*)pkey.data, &ev);
  }

  if (curs != NULL) curs->close(curs);

  if (ret!=DB_NOTFOUND){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }
  return 0;
}

int
do_event_search(char * user, int level, char **argv){
  int ret, i, j;
  int d1,d2;
  int tags[MAX_TAGS];
  event_t mask, ev;
  DBC *curs;
  DBT key  = mk_empty_dbt();
  DBT pkey = mk_empty_dbt();
  DBT pval = mk_empty_dbt();
  char * txt = argv[0];

  mask.tags.ptr = tags;
  if (event_parse(argv+1, &mask)!=0) return -1;

  ret = dbs.events->cursor(dbs.d2ev, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }

  d1 = (mask.date1 >= 0)?  mask.date1:0;
  d2 = (mask.date2 >= 0)?  mask.date2:~0;
  if (d2<d1) d2=d1;

  while ((ret = curs->pget(curs, &key, &pkey, &pval, DB_PREV))==0){
    ev = dbt2event(&pval);
    /* search by date range: */
    if (d1 && d1 > ev.date2) continue;
    if (d2 && d2 < ev.date1) continue;

    /* search by tags: */
    if (mask.ntags > 0){
      int fl=0;
      for (i=0; i<mask.ntags; i++){
        for (j=0; j<ev.ntags; j++){
          if (mask.tags.ptr[i] == ev.tags.ptr[j]) fl=1;
        }
      }
      if (fl==0) continue;
    }

    /* search in text fields: */
    if (strlen(mask.title.ptr)  && !strcasestr(ev.title.ptr,  mask.title.ptr))  continue;
    if (strlen(mask.body.ptr)   && !strcasestr(ev.body.ptr,   mask.body.ptr))   continue;
    if (strlen(mask.people.ptr) && !strcasestr(ev.people.ptr, mask.people.ptr)) continue;
    if (strlen(mask.route.ptr)  && !strcasestr(ev.route.ptr,  mask.route.ptr))  continue;

    /* search in text fields: */
    if (strlen(txt) && (
      !strcasestr(ev.title,  txt) &&
      !strcasestr(ev.body,   txt) &&
      !strcasestr(ev.people, txt) &&
      !strcasestr(ev.route,  txt))) continue;

    event_prn(* (int*)pkey.data, &ev);
  }

  if (curs != NULL) curs->close(curs);

  if (ret!=DB_NOTFOUND){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }
  return 0;
}
