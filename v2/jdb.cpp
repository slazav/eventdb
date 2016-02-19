#include "jdb.h"
using namespace std;

#define DB_DEBUG 0

/********************************************************************/
/* create DBT objects */
DBT
mk_dbt(){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  return ret;
}
DBT
mk_dbt(const char * str){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  ret.data = (char *) str;
  ret.size = strlen(str)+1;
  return ret;
}
DBT
mk_dbt(const string & str){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  ret.data = (char *) str.c_str();
  ret.size = str.length()+1;
  return ret;
}

/********************************************************************/
map<DB*, string> key_names;

/* get key_name*/
string
jbd_get_keyname(DB* dbp){
  if (key_names.find(dbp)==key_names.end())
    throw Err() << "database without key_name";
  return key_names.find(dbp)->second;
}

/* secondary key extractor */
int
jbd_key_extractor(DB *secdb, const DBT *pkey, const DBT *pdata, DBT *skey){
  json_t * root = j_loadstr((const char *)pdata->data);
  std::string k = jbd_get_keyname(secdb);
  std::string v = j_getstr(root, k);
  json_decref(root);
  if (DB_DEBUG)
    cerr << "extract " << k << " : " << v << "\n";
  /* we need to allocate memory for data!
     just repeat the call to json */
  char * str = (char *)malloc(v.length());
  if (!str) throw Err() << "malloc error";
  strcpy(str, v.c_str());
  *skey = mk_dbt(str);
  skey->flags = DB_DBT_APPMALLOC; // data should be freed
  return 0;
}

/********************************************************************/

JDB::JDB(const string & file,
         const string & key_name,
         const int flags, bool dup, cmpfunc_t cmpfunc){
  name = file;
  pdbp = NULL;

  if (DB_DEBUG)
    cerr << "open " << name << "\n";

  /* Initialize the DB handle */
  int ret = db_create(&dbp, NULL, 0);
  if (ret != 0) throw Err() << db_strerror(ret);

  key_names[dbp] = key_name;
  /* setup key duplication if needed */
  if (dup){
    ret = dbp->set_flags(dbp, DB_DUPSORT);
    if (ret != 0) throw Err() << db_strerror(ret);
  }

  /* set key compare function if needed */
  if (cmpfunc){
    ret = dbp->set_bt_compare(dbp, cmpfunc);
    if (ret != 0) throw Err() << db_strerror(ret);
  }

  /* Now open the database */
  ret = dbp->open(dbp,          /* Pointer to the database */
                  NULL,         /* Txn pointer */
                  file.c_str(), /* File name */
                  NULL,         /* DB name */
                  DB_BTREE,     /* Database type (using btree) */
                  flags,        /* Open flags */
                  0);           /* File mode. Using defaults */
  if (ret != 0) throw Err() << db_strerror(ret);
}

JDB::~JDB(){
  if (DB_DEBUG)
    cerr << "close " << name << "\n";
  int ret;
  name="";
  if (dbp){
    key_names.erase(dbp);
    if ((ret=dbp->close(dbp, 0))!=0) throw Err() << db_strerror(ret);
  }
  dbp=NULL;
}

void
JDB::associate(const JDB & prim_db){
  pdbp = prim_db.dbp;
  dbp->associate(pdbp, NULL, dbp, jbd_key_extractor, 0);
  if (DB_DEBUG)
    cerr << "associate " << jbd_get_keyname(dbp) << " -> "
         << jbd_get_keyname(pdbp) << "\n";
}

bool
JDB::exists(const std::string & skey){
  DBT key = mk_dbt(skey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret == DB_NOTFOUND) return false;
  if (ret == 0) return true;
  throw Err() << name << " database: "<< db_strerror(ret);
}

void
JDB::put(json_t * json, bool overwrite){
  /* field key_name is not needed*/
  std::string skey_name = jbd_get_keyname(dbp);
  std::string skey = j_getstr(json, skey_name);
  // j_delstr(json, skey_name);
  /* json -> char* -> database*/
  char *sval = json_dumps(json, 0);
  DBT key = mk_dbt(skey);
  DBT val = mk_dbt(sval);
  if (DB_DEBUG)
    cerr << "put: " << skey << " : " << sval << "\n";
  int ret = dbp->put(dbp, NULL, &key, &val, overwrite? 0:DB_NOOVERWRITE);
  if (ret!=0) throw Err() << name <<  " database: "<< db_strerror(ret);
  free(sval);
}

json_t *
JDB::get(const std::string & skey){
  int ret;
  DBT key  = mk_dbt(skey);
  if (!pdbp){ // primary DB
    DBT key = mk_dbt(skey);
    DBT val = mk_dbt();
    ret = dbp->get(dbp, NULL, &key, &val, 0);
    if (ret != 0) throw Err() << name << " database: " << db_strerror(ret);
    json_t * root = j_loadstr((const char *)val.data);
    /* add primary key to json */
    j_putstr(root, jbd_get_keyname(dbp), skey);
    return root;
  }
  else { // secondary DB
    DBT pkey = mk_dbt();
    DBT pval = mk_dbt();
    ret = dbp->pget(dbp, NULL, &key, &pkey, &pval, 0);
    if (ret != 0) throw Err() << name << " database: " << db_strerror(ret);
    json_t * root = j_loadstr((const char *)pval.data);
    /* add primary key to json */
    j_putstr(root, jbd_get_keyname(pdbp), (const char *)pkey.data);
    return root;
  }
}

bool
JDB::is_empty(){
  DBC *cursorp;
  DBT key = mk_dbt();
  DBT val = mk_dbt();
  /* Get a cursor */
  dbp->cursor(dbp, NULL, &cursorp, 0);
  if (!cursorp) throw Err() << "can't get DB cursor";
  /* Try to get the first value */
  int ret = cursorp->get(cursorp, &key, &val, DB_NEXT);
  cursorp->close(cursorp);
  if (ret == DB_NOTFOUND) return true;
  if (ret == 0) return false;
  throw Err() << "can't use DB cursor";
}
