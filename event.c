#include "event.h"
#include "string.h"
#include "stdlib.h"

/*********************************************************************/
/* Event structure. In the database data is written after
   this structure, and pointers contain offsets from the beginning of
   the structure. */
typedef struct {
  int ctime, mtime;
  int date1, date2;
  char * title,
       * body,
       * people,
       * route,
       * owner;
  int ntags;  /* number of int tags */
  int * tags;
} event_t;

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
           + strlen(event->owner) + 1
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
  strcpy(val.data + ptr, event->owner);
    remove_html(val.data + ptr,  REMOVE_NL);
    ev->owner = NULL + ptr;
    ptr += strlen(event->owner) + 1;

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
  ev.owner  = (char *)dbt->data + ((void*)ev.owner - NULL);
  ev.tags   = (int *)(dbt->data + ((void*)ev.tags - NULL)); /* careful with types!*/
  return ev;
}

/* print event to stdout */
void
print_event(int id, event_t * ev){
  int i;
  printf("<event id=%d>\n", id, ev->date1, ev->date2);
  printf(" <ctime>%d</ctime>\n",   ev->ctime);
  printf(" <mtime>%d</mtime>\n",   ev->mtime);
  printf(" <date1>%d</date1>\n",   ev->date1);
  printf(" <date2>%d</date2>\n",   ev->date2);
  printf(" <title>%s</title>\n",  ev->title);
  printf(" <body>%s</body>\n",    ev->body);
  printf(" <people>%s</people>\n",ev->people);
  printf(" <route>%s</route>\n",  ev->route);
  printf(" <owner>%s</owner>\n",  ev->owner);
  printf(" <tags>");
  for (i=0; i<ev->ntags; i++) printf("%s%d", i==0?"":",", ev->tags[i]);
  printf("</tags>\n");
  printf("</event>\n");
}

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

/* get event information from argv[0..6] and put it to event and tags */
int
event_parse(char **argv, event_t * event, int tags[MAX_TAGS]){
  char *stag, *prev;
  int i;

  event->title  = argv[0];
  event->body   = argv[1];
  event->people = argv[2];
  event->route  = argv[3];
  event->date1 = get_int(argv[4], "date1"); if (event->date1<0) return -1;
  event->date2 = get_int(argv[5], "date2"); if (event->date2<0) return -1;

  if (event->date2<event->date1) event->date2=event->date1;

  stag = argv[6], i=0;
  while (stag && (prev = strsep(&stag, ",:; \n\t"))){
    if (i>MAX_TAGS-1){
      fprintf(stderr, "Too many tags (> %d)\n", MAX_TAGS-1);
      return -1;
    }
    if (strlen(prev)){ tags[i] = atoi(prev);
      if (tags[i]==0){
        fprintf(stderr, "Error: bad tag: %s\n", prev);
        return -1;
      }
    }
    i++;
  }
  event->tags = tags;
  event->ntags = i;
  return 0;
}

/* Check permissions for data modifications:
  ANON - no
  NOAUTH and NORM - owner only,
  MODER and higher - yes */
int
event_mperm_check(unsigned int id, char *user, int level){
  int ret;
  DBT key = mk_uint_dbt(&id);
  DBT val = mk_empty_dbt();
  event_t obj;

  if (level_check(level, LVL_NOAUTH)!=0) return -1;
  if (level >= LVL_MODER) return 0;

  /* get event owner */
  ret = dbs.events->get(dbs.events, NULL, &key, &val, 0);

  if (ret==DB_NOTFOUND){
    fprintf(stderr, "Error: event not found: %d \n", id);
    return ret;
  }
  if (ret!=0){
    fprintf(stderr, "Error: can't get %d event information: %s\n",
      id, db_strerror(ret));
    return ret;
  }
  obj = dbt2event(&val);
  if (strcmp(user, obj.owner)!=0){
    fprintf(stderr, "Error: %s is not allowed to modify data owned by %s\n",
      user, obj.owner);
    return -1;
  }
  return 0;
}

/*********************************************************************/

int
do_event_create(char * user, int level, char **argv){
  unsigned int id;
  int tags[MAX_TAGS];
  event_t ev;
  DBT key, val;
  int ret;

  /* Check permissions: only ANON can't create files */
  if (level_check(level, LVL_NOAUTH)!=0) return -1;

  /* Find last existing event id */
  if ((id = event_last_id(dbs))<0) return -1;
  id++;

  /* Parse arguments and make DBT for new event */
  if (event_parse(argv, &ev, tags)!=0) return -1;
  ev.ctime = time(NULL);
  ev.mtime = ev.ctime;
  ev.owner = user;
  key = mk_uint_dbt(&id);
  val = event2dbt(&ev); /* do free before return! */
  if (val.data==NULL) return -1;

  /* write event */
  ret = dbs.events->put(dbs.events, NULL, &key, &val, DB_NOOVERWRITE);
  if (ret!=0)
    fprintf(stderr, "Error: can't write event: %s\n",
      db_strerror(ret));

  free(val.data);
  return ret;
}

int
do_event_edit(char * user, int level, char **argv){
  unsigned int id;
  int tags[MAX_TAGS];
  event_t ev, oev;
  DBT key, val, oval;
  int ret;

  /* get id from cmdline */
  id = get_uint(argv[0], "event id");
  if (id == 0)  return -1;

  /* Check permissions */
  if (event_mperm_check(id, user, level)!=0) return -1;

  /* check that metadata exists, get old data */
  key = mk_uint_dbt(&id);
  oval = mk_empty_dbt();
  ret = dbs.events->get(dbs.events, NULL, &key, &oval, 0);
  if (ret!=0){
    fprintf(stderr, "Error: can't get event: %s\n",
      db_strerror(ret));
    return ret;
  }
  oev = dbt2event(&oval);

  /* Parse arguments and make DBT for new event */
  if (event_parse(argv+1, &ev, tags)!=0) return -1;
  ev.mtime = time(NULL);
  ev.ctime = oev.ctime;
  ev.owner = oev.owner;
  val = event2dbt(&ev); /* do free before return! */
  if (val.data==NULL) return -1;

  /* write event */
  ret = dbs.events->put(dbs.events, NULL, &key, &val, 0);
  if (ret!=0)
    fprintf(stderr, "Error: can't write event: %s\n",
      db_strerror(ret));

  free(val.data);
  return ret;
}


int
do_event_delete(char * user, int level, char **argv){
  unsigned int id;
  int ret;
  DBT key = mk_uint_dbt(&id);

  /* get id from cmdline */
  id = get_uint(argv[0], "event id");
  if (id == 0)  return -1;

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
  unsigned int id;
  int ret;
  DBT key = mk_uint_dbt(&id);
  DBT val = mk_empty_dbt();
  event_t ev;

  /* get id from cmdline */
  id = get_uint(argv[0], "event id");
  if (id == 0)  return -1;

  ret = dbs.events->get(dbs.events, NULL, &key, &val, 0);
  if (ret==DB_NOTFOUND){
    fprintf(stderr, "Error: event not found: %d \n", id);
    return ret;
  }
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s\n", db_strerror(ret));
    return ret;
  }
  ev = dbt2event(&val);
  print_event(id, &ev);
  return 0;
}

int
do_event_search(char * user, int level, char **argv){
  int ret, i, j;
  unsigned int d1,d2;
  int tags[MAX_TAGS];
  event_t mask, ev;
  DBC *curs;
  DBT key = mk_empty_dbt();
  DBT val = mk_empty_dbt();
  char * txt = argv[0];

  if (event_parse(argv+1, &mask, tags)!=0) return -1;

  ret = dbs.events->cursor(dbs.events, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }

  d1 = (mask.date1 >= 0)?  mask.date1:0;
  d2 = (mask.date2 >= 0)?  mask.date2:~0;
  if (d2<d1) d2=d1;

  while ((ret = curs->get(curs, &key, &val, DB_NEXT))==0){
    ev = dbt2event(&val);
    /* search by date range: */
    if (d1 && d1 > ev.date2) continue;
    if (d2 && d2 < ev.date1) continue;

    /* search by tags: */
    if (mask.ntags > 0){
      int fl=0;
      for (i=0; i<mask.ntags; i++){
        for (j=0; j<ev.ntags; j++){
          if (mask.tags[i] == ev.tags[j]) fl=1;
        }
      }
      if (fl==0) continue;
    }

    /* search in text fields: */
    if (strlen(mask.title)  && !strcasestr(ev.title,  mask.title))  continue;
    if (strlen(mask.body)   && !strcasestr(ev.body,   mask.body))   continue;
    if (strlen(mask.people) && !strcasestr(ev.people, mask.people)) continue;
    if (strlen(mask.route)  && !strcasestr(ev.route,  mask.route))  continue;

    /* search in text fields: */
    if (strlen(txt) && (
      !strcasestr(ev.title,  txt) &&
      !strcasestr(ev.body,   txt) &&
      !strcasestr(ev.people, txt) &&
      !strcasestr(ev.route,  txt))) continue;

    print_event(* (int*)key.data, &ev);
  }

  if (curs != NULL) curs->close(curs);

  if (ret!=DB_NOTFOUND){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }
  return 0;
}
