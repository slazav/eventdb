#include "geo.h"
#include "string.h"
#include "stdlib.h"
#include "errno.h"

/* io buffer */
#define BUFLEN 8192

/*********************************************************************/

/* Geodata structure. In the database data is written after
   this structure, and pointers contain offsets from the beginning of
   the structure. */
typedef struct {
  int mtime;
  int date1, date2;
  int length;
  char * comm,
       * auth,
       * owner;
  int ntags;  /* number of int tags */
  int * tags;
} geo_t;

/*********************************************************************/

/* Build DBT object from geo structure.
   Html <tags> is converted to [tags], '\n' is converted to ' '.
   Space for .data is allocated, this field must be freed after use!
   If error occures val.data is set to NULL! */
DBT
geo2dbt(geo_t * obj){
  DBT val = mk_empty_dbt();
  int ptr, i;
  geo_t * obj1;

  if (obj->date1 < 0) obj->date1=0;
  if (obj->date2 < obj->date1) obj->date2=obj->date1;

  /* calculate data size and build data structure */
  val.size = sizeof(geo_t)   /* static fields + pointers */
           + strlen(obj->comm) + 1 /* including \0*/
           + strlen(obj->auth) + 1
           + strlen(obj->owner) + 1
           + sizeof(int) * obj->ntags;
  val.data = malloc(val.size);
  if (val.data==NULL){
    fprintf(stderr, "Error: can't allocate memory\n");
    return val;
  }
  /* copy event to the header of val.data */
  obj1 = (geo_t *)val.data;
  *obj1 = *obj;

  ptr=sizeof(geo_t);
  strcpy(val.data + ptr, obj->comm);
    remove_html(val.data + ptr,  REMOVE_NL);
    obj1->comm = NULL + ptr;
    ptr += strlen(obj->comm) + 1;
  strcpy(val.data + ptr, obj->auth);
    remove_html(val.data + ptr,  KEEP_NL);
    obj1->auth = NULL + ptr;
    ptr += strlen(obj->auth) + 1;
  strcpy(val.data + ptr, obj->owner);
    remove_html(val.data + ptr,  REMOVE_NL);
    obj1->owner = NULL + ptr;
    ptr += strlen(obj->owner) + 1;

  obj1->tags = (int *)(NULL + ptr);
  for (i=0; i<obj->ntags; i++){
    * (int *)(val.data + ptr) = obj->tags[i];
    ptr+=sizeof(int);
  }
  return val;
}

/* Convert DBT structure to geo. Data is not copied! */
geo_t
dbt2geo(DBT * dbt){
  geo_t obj = * (geo_t *)dbt->data;
  /* Overwrite pointers to absolute values */
  obj.comm  = (char *)dbt->data + ((void*)obj.comm - NULL); /* (char*) + (int)(void*-void*) */
  obj.auth  = (char *)dbt->data + ((void*)obj.auth - NULL);
  obj.owner = (char *)dbt->data + ((void*)obj.owner - NULL);
  obj.tags   = (int *)(dbt->data + ((void*)obj.tags - NULL)); /* careful with types! */
  return obj;
}

/* print data to stdout */
void
print_geo(char * fname, geo_t * obj){
  int i;
  printf("<geo file=\"%s\">\n", fname);
  printf(" <mtime>%d</mtime>\n",   obj->mtime);
  printf(" <date1>%d</date1>\n",   obj->date1);
  printf(" <date2>%d</date2>\n",   obj->date2);
  printf(" <length>%d</length>\n", obj->length);
  printf(" <comm>%s</comm>\n",     obj->comm);
  printf(" <auth>%s</auth>\n",     obj->auth);
  printf(" <owner>%s</owner>\n",   obj->owner);
  printf(" <tags>");
  for (i=0; i<obj->ntags; i++) printf("%s%d", i==0?"":",", obj->tags[i]);
  printf("</tags>\n");
  printf("</geo>\n");
}

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
  if (n>GEO_MAX_FNAME){
    fprintf(stderr, "Error: too long name (>%d chars)\n", GEO_MAX_FNAME);
    return -1;
  }
  if (strspn(fname, accept)!=n){
    fprintf(stderr, "Error: only a-z,A-Z,0-9, . and _ characters are accepted\n");
    return -1;
  }
  return 0;
}


/* Build absolute filename (check_fname included)*/
char *
build_path(const char *fname){
  static char path[BUFLEN];
  int l1, l2;

  if (check_fname(fname)!=0) return NULL;
  l1 = strlen(GEO_FDIR);
  l2 = strlen(fname);
  if (l1 + l2 + 2 > BUFLEN){
    fprintf(stderr, "Error: filename buffer is too short\n");
    return NULL;
  }
  strcpy(path, GEO_FDIR);
  path[l1] = '/';
  strcpy(path+l1+1, fname);
  return path;
}

/* store file */
int
put_file(char * fname, int overwrite){
  char *path, buf[BUFLEN];
  FILE *F;
  int count;

  path=build_path(fname);
  if (path==NULL) return -1;

  /* create file (error if file exists in non-overwrite mode) */
  F = fopen(path, overwrite? "w":"wx");
  if (F == NULL){
    perror("Error");
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
    if (count > GEO_MAX_FSIZE){
      fclose(F);
      fprintf(stderr, "Error: file too large (> %d bytes)\n", GEO_MAX_FSIZE);
      if (unlink(path)!=0) perror("Error");
      return -1;
    }
  }
  if (ferror(stdin) || ferror(F)){
    perror("Error");
    return -1;
  }
  fclose(F);
}


/* get geodata information from argv[0..5] and put it to geo and tags */
int
geo_parse(char **argv, geo_t * geo, int tags[MAX_TAGS]){
  char *stag, *prev;
  int i;

  geo->comm  = argv[0];
  geo->auth  = argv[1];
  geo->date1 = get_int(argv[2], "date1");   if (geo->date1<0)  return -1;
  geo->date2 = get_int(argv[3], "date2");   if (geo->date2<0)  return -1;
  geo->length = get_int(argv[4], "length"); if (geo->length<0) return -1;

  stag = argv[5], i=0;
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
  geo->tags = tags;
  geo->ntags = i;
  return 0;
}

/* Check permissions for data modifications:
  ANON - no
  NOAUTH and NORM - owner only,
  MODER and higher - yes */
int
geo_mperm_check(char * fname, char *user, int level){
  int ret;
  DBT key = mk_string_dbt(fname);
  DBT val = mk_empty_dbt();
  geo_t obj;

  if (level_check(level, LVL_NOAUTH)!=0) return -1;
  if (level >= LVL_MODER) return 0;

  /* get track's owner */
  ret = dbs.tracks->get(dbs.tracks, NULL, &key, &val, 0);

  if (ret==DB_NOTFOUND){
    fprintf(stderr, "Error: file not found: %s \n", fname);
    return ret;
  }
  if (ret!=0){
    fprintf(stderr, "Error: can't get file information %s: %s\n",
      fname, db_strerror(ret));
    return ret;
  }
  obj = dbt2geo(&val);
  if (strcmp(user, obj.owner)!=0){
    fprintf(stderr, "Error: %s is not allowed to modify data owned by %s\n",
      user, obj.owner);
    return -1;
  }
  return 0;
}

/*********************************************************************/
/* ACTIONS */

int
do_geo_create(char * user, int level, char **argv){
  int tags[MAX_TAGS];
  char * fname = argv[0];
  geo_t geo;
  int ret;
  DBT key, val;

  /* Check permissions: only ANON can't create files */
  if (level_check(level, LVL_NOAUTH)!=0) return -1;

  /* Parse arguments and make db key-val pair*/
  if (geo_parse(argv+1, &geo, tags)) return -1;
  geo.mtime = time(NULL);
  geo.owner = user;
  key = mk_string_dbt(fname);
  val = geo2dbt(&geo); /* do free before return! */

  /* put file */
  if (put_file(fname, 0)!=0) return -1;

  /* write metadata */
  key = mk_string_dbt(fname);
  val = geo2dbt(&geo); /* do free before return! */
  if (val.data==NULL) return -1;

  ret = dbs.tracks->put(dbs.tracks, NULL, &key, &val, DB_NOOVERWRITE);

  if (ret!=0)
    fprintf(stderr, "Error: can't write geodata information: %s\n",
      db_strerror(ret));

  free(val.data);
  return ret;
}

int
do_geo_replace(char * user, int level, char **argv){
  char *fname = argv[0];

  /* Check permissions and replace file */
  return geo_mperm_check(fname, user, level) ||
         put_file(fname, 1);
}

int
do_geo_delete(char * user, int level, char **argv){
  int ret;
  char *path;
  char *fname = argv[0];
  DBT key = mk_string_dbt(fname);

  /* Check permissions */
  if (geo_mperm_check(fname, user, level)!=0) return -1;

  /* Check filename and build path */
  path=build_path(fname);
  if (path==NULL) return -1;

  /* Delete metadata */
  ret = dbs.tracks->del(dbs.tracks, NULL, &key, 0);
  if (ret!=0)
    fprintf(stderr, "Error: can't delete geodata: %s\n",
      db_strerror(ret));

  /* Delete file if it exists */
  if (unlink(path)!=0){
    perror("Error");
    return -1;
  }
  return ret;
}

int
do_geo_edit(char * user, int level, char **argv){
  int tags[MAX_TAGS];
  char * fname = argv[0];
  geo_t geo;
  int ret;
  DBT key, val;

  /* Check permissions */
  if (geo_mperm_check(fname, user, level)!=0) return -1;

  /* Parse arguments and make db key-val pair*/
  if (geo_parse(argv+1, &geo, tags)) return -1;
  geo.mtime = time(NULL);
  geo.owner = user;
  key = mk_string_dbt(fname);
  val = geo2dbt(&geo); /* do free before return! */


  /* check that metadata exists */
  ret = dbs.tracks->get(dbs.tracks, NULL, &key, &val, 0);
  if (ret!=0){
    fprintf(stderr, "Error: can't get geodata information: %s\n",
      db_strerror(ret));
    return ret;
  }

  /* write metadata */
  ret = dbs.tracks->put(dbs.tracks, NULL, &key, &val, 0);
  if (ret!=0)
    fprintf(stderr, "Error: can't write geodata information: %s\n",
      db_strerror(ret));

  free(val.data);
  return ret;
}


int
do_geo_show(char * user, int level, char **argv){
  char *fname = argv[0];
  int ret;
  DBT key = mk_string_dbt(fname);
  DBT val = mk_empty_dbt();
  geo_t obj;
  ret = dbs.tracks->get(dbs.tracks, NULL, &key, &val, 0);
  if (ret==DB_NOTFOUND){
    fprintf(stderr, "Error: file not found: %s \n", fname);
    return ret;
  }
  if (ret!=0){
    fprintf(stderr, "Error: can't get file information %s: %s\n",
      fname, db_strerror(ret));
    return ret;
  }
  obj = dbt2geo(&val);
  print_geo(fname, &obj);
  return 0;
}

int
do_geo_list(char * user, int level, char **argv){

  DBC *curs;
  DBT key = mk_empty_dbt();
  DBT val = mk_empty_dbt();
  geo_t obj;
  int ret;

  ret = dbs.tracks->cursor(dbs.tracks, NULL, &curs, 0);
  if (ret!=0){
    fprintf(stderr, "Error: can't get info from database: %s \n",
      db_strerror(ret));
    return ret;
  }

  while ((ret = curs->get(curs, &key, &val, DB_NEXT))==0){
    obj = dbt2geo(&val);
    print_geo((char *)key.data, &obj);
  }

  if (curs != NULL) curs->close(curs);

  if (ret!=DB_NOTFOUND){
    fprintf(stderr, "Error: can't get file info from database: %s \n",
      db_strerror(ret));
    return ret;
  }
  return 0;
}
