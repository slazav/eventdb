#include "actions.h"
#include "event.h"
#include "string.h"
#include "stdlib.h"

/*********************************************************************/
/* Link structure. In the database data is written after
   this structure, and pointers contain offsets from the beginning of
   the structure. */
typedef struct {
  int ctime, mtime;
  int eventid;
  char * url,
       * text,
       * owner;
  int ntags;  /* number of int tags */
  int * tags;
} link_t;

/*********************************************************************/
/* Build DBT object from link structure.
   Html <tags> is converted to [tags], '\n' is converted to ' '
   Space for .data is allocated, this field must be freed after use!
   If error occures val.data is set to NULL! */
DBT
link2dbt(link_t * link){
  DBT val = mk_empty_dbt();
  int ptr, i;
  link_t * obj;

  /* calculate data size and build data structure */
  val.size = sizeof(link_t)   /* static fields + pointers */
           + strlen(link->url) + 1 /* including \0*/
           + strlen(link->text) + 1
           + strlen(link->owner) + 1
           + sizeof(int) * link->ntags;
  val.data = malloc(val.size);
  if (val.data==NULL){
    fprintf(stderr, "Error: can't allocate memory\n");
    return val;
  }
  /* copy link to the header of val.data */
  obj = (link_t *)val.data;
  *obj = *link;

  ptr=sizeof(link_t);
  strcpy(val.data + ptr, link->url); /* copy data */
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->url = NULL + ptr;
    ptr += strlen(link->url) + 1;
  strcpy(val.data + ptr, link->text);
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->text = NULL + ptr;
    ptr += strlen(link->text) + 1;
  strcpy(val.data + ptr, link->owner);
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->owner = NULL + ptr;
    ptr += strlen(link->owner) + 1;

  obj->tags = (int *)(NULL + ptr);
  for (i=0; i<link->ntags; i++){
    * (int *)(val.data + ptr) = link->tags[i];
    ptr+=sizeof(int);
  }
  return val;
}

/* Convert DBT structure to link. Data is not copied! */
link_t
dbt2link(const DBT * dbt){
  link_t obj = * (link_t *)dbt->data;
  /* Overwrite pointers to absolute values */
  obj.url   = (char *)dbt->data + ((void*)obj.url   - NULL); /* (char*) + (int)(void*-void*) */
  obj.text  = (char *)dbt->data + ((void*)obj.text  - NULL);
  obj.owner = (char *)dbt->data + ((void*)obj.owner - NULL);
  obj.tags  = (int *)(dbt->data + ((void*)obj.tags  - NULL)); /* careful with types!*/
  return obj;
}

/* eventid extractor for the secondary db */
int
db_link_eventid(DB *secdb, const DBT *pkey, const DBT *pdata, DBT *skey){
  static int val;
  link_t obj = dbt2link(pdata);
  memset(skey, 0, sizeof(DBT));
  val=obj.eventid;
  skey->data = &val;
  skey->size = sizeof(int);
  return 0;
}

/* Print link to stdout. */
void
link_prn(int id, link_t * obj){
  int i;
  printf("<link id=%d>\n", id);
  printf(" <ctime>%d</ctime>\n",     obj->ctime);
  printf(" <mtime>%d</mtime>\n",     obj->mtime);
  printf(" <eventid>%d</eventid>\n", obj->eventid);
  printf(" <url>%s</url>\n",         obj->url);
  printf(" <text>%s</text>\n",       obj->text);
  printf(" <owner>%s</owner>\n",     obj->owner);
  printf(" <tags>");
  for (i=0; i<obj->ntags; i++) printf("%s%d", i==0?"":",", obj->tags[i]);
  printf("</tags>\n");
  printf("</link>\n");
}

/* Get last link id in the database. */
int
link_last_id(){
  int id, ret;
  DBC *curs;
  DBT key = mk_empty_dbt();
  DBT val = mk_empty_dbt();

  ret = dbs.links->cursor(dbs.links, NULL, &curs, 0)==0? 0:-1;

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

/* Get link information from argv[0..2] and put it to link and tags */
/* NOTE: link.tags must be a valid pointer to int[MAX_TAGS]*/
int
link_parse(char **argv, link_t * link){
  link->url   = argv[0];
  link->text  = argv[1];
  if ((link->ntags=get_tags(argv[2], link->tags)) <0) return -1;
  return 0;
}

int
link_put(int id, link_t *obj, int flags){
  DBT key = mk_uint_dbt(&id);
  DBT val = link2dbt(obj); /* do free before return! */
  int ret;

  if (val.data==NULL) return -1;

  /* write link */
  ret = dbs.links->put(dbs.links, NULL, &key, &val, flags);
  if (ret!=0)
    fprintf(stderr, "Error: can't write link: %s\n",
      db_strerror(ret));
  free(val.data);
  return ret;
}

int
link_get(int id, link_t *obj){
  DBT key = mk_uint_dbt(&id);
  DBT val = mk_empty_dbt();
  int ret = dbs.links->get(dbs.links, NULL, &key, &val, 0);
  if (ret==DB_NOTFOUND){
    fprintf(stderr, "Error: link not found: %d \n", id);
    return ret;
  }
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s\n", db_strerror(ret));
    return ret;
  }
  *obj = dbt2link(&val);
  return 0;
}

/* Check permissions for data modifications:
  ANON - no
  NOAUTH and NORM - owner of link or event,
  MODER and higher - yes */
int
link_mperm_check(int id, char *user, int level){
  int ret;
  DBT key = mk_uint_dbt(&id);
  DBT val = mk_empty_dbt();
  link_t  ll;
  event_t ev;

  if (level_check(level, LVL_NOAUTH)!=0) return -1;
  if (level >= LVL_MODER) return 0;

  /* check link owner */
  if (link_get(id, &ll)!=0) return -1;
  if (strcmp(user, ll.owner)==0) return 0;

  /* check event owner */
  if (event_get(ll.eventid, &ev)!=0) return -1;
  if (strcmp(user, ev.owner)==0) return 0;

  fprintf(stderr, "Error: %s is not allowed to modify data\n", user);
  return -1;
}

/*********************************************************************/
/* Actions */
int
do_link_create(char * user, int level, char **argv){
  int id;
  int tags[MAX_TAGS];
  link_t obj;
  event_t ev;

  /* Check permissions: only ANON can't create links */
  if (level_check(level, LVL_NOAUTH)!=0) return -1;

  /* Find last existing link id */
  if ((id = link_last_id(dbs))<0) return -1;
  id++;

  /* check event */
  obj.eventid = get_int(argv[0], "event id");
  if (obj.eventid<0) return -1;
  if (event_get(obj.eventid, &ev)!=0) return -1;

  /* Parse arguments and make DBT for new link */
  obj.ctime = time(NULL);
  obj.mtime = obj.ctime;
  obj.owner = user;
  obj.tags  = tags;
  if (link_parse(argv+1, &obj)!=0) return -1;

  if (link_put(id, &obj, DB_NOOVERWRITE)!=0) return -1;
  printf("%d\n", id);
  return 0;
}

int
do_link_edit(char * user, int level, char **argv){
  int id;
  int tags[MAX_TAGS];
  link_t obj, oobj;

  /* get id from cmdline */
  id = get_int(argv[0], "link id");
  if (id <= 0)  return -1;

  /* Check permissions */
  if (link_mperm_check(id, user, level)!=0) return -1;

  if (link_get(id, &oobj)!=0) return -1;

  /* Parse arguments and make DBT for new link */
  obj.eventid = oobj.eventid;
  obj.mtime = time(NULL);
  obj.ctime = oobj.ctime;
  obj.owner = oobj.owner;
  obj.tags  = tags;
  if (link_parse(argv+1, &obj)!=0) return -1;

  return link_put(id, &obj, 0);
}


int
do_link_delete(char * user, int level, char **argv){
  int id;
  int ret;
  DBT key = mk_uint_dbt(&id);

  /* get id from cmdline */
  id = get_int(argv[0], "link id");
  if (id <= 0)  return -1;

  /* Check permissions */
  if (link_mperm_check(id, user, level)!=0) return -1;

  /* delete link */
  ret = dbs.links->del(dbs.links, NULL, &key, 0);
  if (ret==DB_NOTFOUND){
    fprintf(stderr, "Error: link not found: %d \n", id);
    return ret;
  }
  else if (ret!=0){
    fprintf(stderr, "Error: database error: %s\n", db_strerror(ret));
    return ret;
  }
  return ret;
}

int
do_link_show(char * user, int level, char **argv){
  int id;
  link_t obj;

  /* get id from cmdline */
  id = get_int(argv[0], "link id");
  if (id == 0)  return -1;

  if (link_get(id, &obj)!=0) return -1;
  link_prn(id, &obj);
  return 0;
}

int
do_link_list_ev(char * user, int level, char **argv){
  DBC *curs;
  int id;
  DBT key  = mk_uint_dbt(&id);
  DBT pkey = mk_empty_dbt();
  DBT pval = mk_empty_dbt();
  link_t obj;
  int ret;

  /* get event id from cmdline */
  id = get_int(argv[0], "link id");
  if (id <= 0)  return -1;

  ret = dbs.links->cursor(dbs.e2ln, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }

  ret = curs->pget(curs, &key, &pkey, &pval, DB_SET);
  while (ret==0){
    obj = dbt2link(&pval);
    link_prn(* (int*)pkey.data, &obj);
    ret = curs->pget(curs, &key, &pkey, &pval, DB_NEXT_DUP);
  }

  if (curs != NULL) curs->close(curs);

  if (ret!=DB_NOTFOUND){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }
  return 0;
}


int
do_link_list(char * user, int level, char **argv){
  DBC *curs;
  DBT key  = mk_empty_dbt();
  DBT val  = mk_empty_dbt();
  link_t obj;
  int ret;

  ret = dbs.links->cursor(dbs.links, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }

  while ((ret = curs->get(curs, &key, &val, DB_NEXT))==0){
    obj = dbt2link(&val);
    link_prn(* (int*)key.data, &obj);
  }

  if (curs != NULL) curs->close(curs);

  if (ret!=DB_NOTFOUND){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }
  return 0;
}

int
do_link_search(char * user, int level, char **argv){
  int ret, i, j;
  int tags[MAX_TAGS];
  link_t mask, obj;
  DBC *curs;
  DBT key  = mk_empty_dbt();
  DBT val = mk_empty_dbt();

  mask.tags=tags;
  if (link_parse(argv, &mask)!=0) return -1;

  ret = dbs.links->cursor(dbs.links, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }

  while ((ret = curs->get(curs, &key, &val, DB_NEXT))==0){
    obj = dbt2link(&val);

    /* search by tags: */
    if (mask.ntags > 0){
      int fl=0;
      for (i=0; i<mask.ntags; i++){
        for (j=0; j<obj.ntags; j++){
          if (mask.tags[i] == obj.tags[j]) fl=1;
        }
      }
      if (fl==0) continue;
    }

    /* search in text fields: */
    if (strlen(mask.url)  && !strcasestr(obj.url,  mask.url))  continue;
    if (strlen(mask.text) && !strcasestr(obj.text, mask.text)) continue;

    link_prn(* (int*)key.data, &obj);
  }

  if (curs != NULL) curs->close(curs);

  if (ret!=DB_NOTFOUND){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }
  return 0;
}
