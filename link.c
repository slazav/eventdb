#include "actions.h"
#include "event.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

/* folder for local files*/
#ifdef MCCME
#define LOCAL_FDIR "/home/slazav/CH/gps"
#else
#define LOCAL_FDIR "./gps"
#endif

/* limits: local filename size, file size */
#define LOCAL_MAX_FNAME 32
#define LOCAL_MAX_FSIZE 100000

/* io buffer */
#define BUFLEN 8192
#define PATHLEN 1024


/*********************************************************************/
/* Build DBT object from general entry.
   Space for .data is allocated, this field must be freed after use!
   If error occures val.data is set to NULL! */
DBT
e2dbt(db_entry_t * ent){
  DBT val = mk_empty_dbt();
  int ptr, i;
  link_t * obj;

  /* calculate data size and build data structure */
  val.size = sizeof(link_t)   /* static fields + pointers */
           + strlen(link->url.ptr) + 1 /* including \0*/
           + strlen(link->text.ptr) + 1
           + strlen(link->auth.ptr) + 1
           + strlen(link->owner.ptr) + 1
           + sizeof(int32_t) * link->ntags;
  val.data = malloc(val.size);
  if (val.data==NULL){
    fprintf(stderr, "Error: can't allocate memory\n");
    return val;
  }
  /* copy link to the header of val.data */
  obj = (link_t *)val.data;
  *obj = *link;

  ptr=sizeof(link_t);
  strcpy(val.data + ptr, link->url.ptr); /* copy data */
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->url.off = ptr;
    ptr += strlen(link->url.ptr) + 1;
  strcpy(val.data + ptr, link->text.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->text.off = ptr;
    ptr += strlen(link->text.ptr) + 1;
  strcpy(val.data + ptr, link->auth.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->auth.off = ptr;
    ptr += strlen(link->auth.ptr) + 1;
  strcpy(val.data + ptr, link->owner.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->owner.off = ptr;
    ptr += strlen(link->owner.ptr) + 1;

  obj->tags.off = ptr;
  for (i=0; i<link->ntags; i++){
    * (int32_t *)(val.data + ptr) = link->tags.ptr[i];
    ptr+=sizeof(int32_t);
  }
  return val;
}

/*********************************************************************/
/* Link structure. In the database data is written after
   this structure, and pointers contain offsets from the beginning of
   the structure. Don't put here arch-dependent things such as int or char*! */
typedef struct {
  int32_t ctime, mtime;
  int32_t eventid;
  int8_t local; /* 0 or 1 -- is local file stored?*/
  char_ptr url,
           text,
           auth,
           owner;
  int32_t ntags;  /* number of int tags */
  int_ptr tags;
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
           + strlen(link->url.ptr) + 1 /* including \0*/
           + strlen(link->text.ptr) + 1
           + strlen(link->auth.ptr) + 1
           + strlen(link->owner.ptr) + 1
           + sizeof(int32_t) * link->ntags;
  val.data = malloc(val.size);
  if (val.data==NULL){
    fprintf(stderr, "Error: can't allocate memory\n");
    return val;
  }
  /* copy link to the header of val.data */
  obj = (link_t *)val.data;
  *obj = *link;

  ptr=sizeof(link_t);
  strcpy(val.data + ptr, link->url.ptr); /* copy data */
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->url.off = ptr;
    ptr += strlen(link->url.ptr) + 1;
  strcpy(val.data + ptr, link->text.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->text.off = ptr;
    ptr += strlen(link->text.ptr) + 1;
  strcpy(val.data + ptr, link->auth.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->auth.off = ptr;
    ptr += strlen(link->auth.ptr) + 1;
  strcpy(val.data + ptr, link->owner.ptr);
    remove_html(val.data + ptr,  REMOVE_NL);
    obj->owner.off = ptr;
    ptr += strlen(link->owner.ptr) + 1;

  obj->tags.off = ptr;
  for (i=0; i<link->ntags; i++){
    * (int32_t *)(val.data + ptr) = link->tags.ptr[i];
    ptr+=sizeof(int32_t);
  }
  return val;
}

/* Convert DBT structure to link. Data is not copied! */
link_t
dbt2link(const DBT * dbt){
  link_t obj = * (link_t *)dbt->data;
  /* Overwrite pointers to absolute values */
  obj.url.ptr   = (char *)dbt->data + obj.url.off; /* (char*) + (int) */
  obj.text.ptr  = (char *)dbt->data + obj.text.off;
  obj.auth.ptr  = (char *)dbt->data + obj.auth.off;
  obj.owner.ptr = (char *)dbt->data + obj.owner.off;
  obj.tags.ptr  = (int *)(dbt->data + obj.tags.off);
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
  printf(" <link id=%d>\n", id);
  printf("  <ctime>%d</ctime>\n",     obj->ctime);
  printf("  <mtime>%d</mtime>\n",     obj->mtime);
  printf("  <eventid>%d</eventid>\n", obj->eventid);
  printf("  <local>%d</local>\n",     obj->local);
  printf("  <url>%s</url>\n",         obj->url.ptr);
  printf("  <text>%s</text>\n",       obj->text.ptr);
  printf("  <auth>%s</auth>\n",       obj->auth.ptr);
  printf("  <owner>%s</owner>\n",     obj->owner.ptr);
  printf("  <tags>");
  for (i=0; i<obj->ntags; i++)
    printf("%s%d", i==0?"":",", obj->tags.ptr[i]);
  printf("</tags>\n");
  printf(" </link>\n");
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
  link->url.ptr   = argv[0];
  link->text.ptr  = argv[1];
  link->auth.ptr  = argv[2];
  link->local = get_int(argv[3], "local flag"); if (link->local<0) return -1;
  if ((link->ntags=get_tags(argv[4], link->tags.ptr)) <0) return -1;
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
  if (strcmp(user, ll.owner.ptr)==0) return 0;

  /* check event owner */
  if (event_get(ll.eventid, &ev)!=0) return -1;
  if (strcmp(user, ev.owner.ptr)==0) return 0;

  fprintf(stderr, "Error: %s is not allowed to modify data\n", user);
  return -1;
}

/* print links for one event */
int
list_event_links(int eventid){
  DBC *curs;
  DBT key  = mk_uint_dbt(&eventid);
  DBT pkey = mk_empty_dbt();
  DBT pval = mk_empty_dbt();
  link_t obj;
  int ret;

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

/*********************************************************************/
/* Operations with local files */

int
check_fname(const char * fname){
  const char accept[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789-_.";
  int i, n=strlen(fname);
  if (n<1){
    fprintf(stderr, "Error: empty name\n");
    return -1;
  }
  if (n>LOCAL_MAX_FNAME){
    fprintf(stderr, "Error: too long name (>%d chars)\n", LOCAL_MAX_FNAME);
    return -1;
  }
  if (strspn(fname, accept)!=n){
    fprintf(stderr, "Error: only a-z,A-Z,0-9, . and _ characters are accepted\n");
    return -1;
  }
  return 0;
}

/* Build absolute filename (check_fname included)*/
/* You need to free path manually after use! */
char *
build_path(const char *fname){
  char *path;
  int l1, l2;

  if (check_fname(fname)!=0) return NULL;
  l1 = strlen(LOCAL_FDIR);
  l2 = strlen(fname);
  path = (char *)malloc(l1 + l2 + 2);
  if (path==NULL){
    fprintf(stderr, "Error: can't allocate memory\n");
    return NULL;
  }
  strcpy(path, LOCAL_FDIR);
  path[l1] = '/';
  strcpy(path+l1+1, fname);
  return path;
}

/* store file */
int
file_put(char * fname, int overwrite){
  char *path, buf[BUFLEN];
  FILE *F;
  int count;

  path=build_path(fname); /* Do free after use! */
  if (path==NULL) return -1;

  /* create file (error if file exists in non-overwrite mode) */
  F = fopen(path, overwrite? "w":"wx");
  if (F == NULL){
    perror("Error");
    free(path);
    return -1;
  }
  /* copy stdin to the file */
  count=0;
  while (!feof(stdin)){
    int i;
    i = fread(buf, 1, sizeof(buf), stdin);
    if (i==0) break;
    i = fwrite(buf, 1, i, F);
    if (i==0) break;
    count +=i;
    if (count > LOCAL_MAX_FSIZE){
      fclose(F);
      fprintf(stderr, "Error: file too large (> %d bytes)\n", LOCAL_MAX_FSIZE);
      if (unlink(path)!=0) perror("Error");
      free(path);
      return -1;
    }
  }
  free(path);

  if (ferror(stdin) || ferror(F)){
    perror("Error");
    return -1;
  }
  fclose(F);
  return 0;
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
  obj.owner.ptr = user;
  obj.tags.ptr  = tags;
  if (link_parse(argv+1, &obj)!=0) return -1;

  /* put file */
  if (obj.local && file_put(obj.url.ptr, 0)!=0) return -1;

  /* write metadata */
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
  obj.tags.ptr  = tags;
  if (link_parse(argv+1, &obj)!=0) return -1;
  obj.local = oobj.local; /* can't change local flag! */

  /* move file if needed */
  if (oobj.local && strcmp(oobj.url.ptr,obj.url.ptr)!=0){
    char *path, *opath;
    struct stat stat_buf;
    int ret;
    path = build_path(obj.url.ptr); /* Do free after use */
    if (path==NULL) return -1;
    opath = build_path(oobj.url.ptr); /* Do free after use */
    if (opath==NULL) {free(path); return -1;}
    /* prevent from overwriting */
    if (stat(path, &stat_buf)==0){
      fprintf(stderr, "Error: file exists: %s\n", path);
      free(path); free(opath);
      return -1;
    }
    if (rename(opath, path)!=0){
      perror("Error");
      free(path); free(opath);
      return -1;
    }
    free(path); free(opath);
  }

  return link_put(id, &obj, 0);
}


int
do_link_delete(char * user, int level, char **argv){
  int id;
  int ret;
  link_t oobj;
  DBT key = mk_uint_dbt(&id);

  /* get id from cmdline */
  id = get_int(argv[0], "link id");
  if (id <= 0)  return -1;

  /* Check permissions */
  if (link_mperm_check(id, user, level)!=0) return -1;

  /* get old link */
  if (link_get(id, &oobj)!=0) return -1;

  /* delete local file if needed*/
  if (oobj.local){
    char * path=build_path(oobj.url.ptr); /* free after use! */
    if (path==NULL) return -1;
    if (unlink(path)!=0){
      perror("Error");
      free(path);
      return -1;
    }
    free(path);
  }

  /* delete link */
  ret = dbs.links->del(dbs.links, NULL, &key, 0);
  if (ret==DB_NOTFOUND){
    fprintf(stderr, "Error: link not found: %d\n", id);
    return ret;
  }
  else if (ret!=0){
    fprintf(stderr, "Error: database error: %s\n", db_strerror(ret));
    return ret;
  }

  return ret;
}

int
do_link_replace(char * user, int level, char **argv){
  int id;
  link_t oobj;

  /* get id from cmdline */
  id = get_int(argv[0], "link id");
  if (id <= 0)  return -1;

  /* Check permissions */
  if (link_mperm_check(id, user, level)!=0) return -1;

  /* get link data */
  if (link_get(id, &oobj)!=0) return -1;

  if (!oobj.local){
    fprintf(stderr, "Error: non-local file.\n");
    return -1;
  }

  return file_put(oobj.url.ptr, 1);
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
  int id = get_int(argv[0], "link id");
  if (id <= 0)  return -1;
  return list_event_links(id);
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

  mask.tags.ptr=tags;
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
          if (mask.tags.ptr[i] == obj.tags.ptr[j]) fl=1;
        }
      }
      if (fl==0) continue;
    }

    /* search in text fields: */
    if (strlen(mask.url.ptr)  && !strcasestr(obj.url.ptr,  mask.url.ptr))  continue;
    if (strlen(mask.text.ptr) && !strcasestr(obj.text.ptr, mask.text.ptr)) continue;
    if (strlen(mask.auth.ptr) && !strcasestr(obj.auth.ptr, mask.auth.ptr)) continue;

    link_prn(* (int*)key.data, &obj);
  }

  if (curs != NULL) curs->close(curs);

  if (ret!=DB_NOTFOUND){
    fprintf(stderr, "Error: database error: %s \n", db_strerror(ret));
    return ret;
  }
  return 0;
}
