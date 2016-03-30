#include <string>
#include <cassert>
#include <map>
#include <set>
#include <iostream>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>
#include "jsondb.h"

/********************************************************************/

/* I keep a key name for any secondary database in the
   global map to use the global extractor function. */
std::map<DB*, std::string> jsondb_sec_keys;

/* Universal key extractor: extracts string or array of strings from json object */
int jsonbd_key_extractor(DB *secdb, const DBT *pkey, const DBT *pdata, DBT *skey);

/********************************************************************/

// Type for integer keys. We need > 64bit for ms time in logs
typedef unsigned long long int jsondb_ikey_t;

/********************************************************************/
// create DBT objects
DBT
mk_dbt(){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  return ret;
}
DBT
mk_dbt(const jsondb_ikey_t * i){
  DBT ret = mk_dbt();
  ret.data = (void *)i;
  ret.size = sizeof(jsondb_ikey_t);
  return ret;
}
DBT
mk_dbt(const char * str){
  DBT ret = mk_dbt();
  ret.data = (char *) str;
  ret.size = strlen(str)+1;
  return ret;
}
DBT
mk_dbt(const std::string & str){
  DBT ret = mk_dbt();
  ret.data = (char *) str.data();
  ret.size = str.length()+1;
  return ret;
}


/********************************************************************/
// The main JsonDB class

/************************************/
// database handles, refcounter and memory menegement.

void
JsonDB::copy(const JsonDB & other){
  dbp   = other.dbp;
  fname = other.fname;
  flags = other.flags;
  intkeys = other.intkeys;
  sec_dbp = other.sec_dbp;
  refcounter = other.refcounter;
  (*refcounter)++;
  assert(*refcounter >0);
}

void
JsonDB::destroy(void){
  (*refcounter)--;
  if (*refcounter<=0){
    // close secondary dbs
    std::map<std::string, DB*>::const_iterator i;
    for (i=sec_dbp.begin(); i!=sec_dbp.end(); i++){
      jsondb_sec_keys.erase(i->second);
      int ret=i->second->close(i->second, 0);
      if (ret!=0) _dberr(ret);
    }
    sec_dbp.clear();
    // close primary db
    int ret = dbp->close(dbp, 0);
    if (ret!=0) _dberr(ret);
    delete refcounter;
  }
}

/************************************/
/* Main constructor
     file    -- database filename
     flags   -- DB_CREATE | DB_EXCL | DB_RDONLY | DB_TRUNCATE ...
     intkeys -- db use integer/string keys
 */
JsonDB::JsonDB(const std::string & fname_,
               const bool intkeys_,
               const int flags_):
           fname(fname_), flags(flags_), intkeys(intkeys_){

  /* Initialize the DB handle */
  int ret = db_create(&dbp, NULL, 0);
  if (ret != 0) _dberr(ret);

  /* setup key duplication if needed */
  // if (dup){
  //   ret = dbp->set_flags(dbp, DB_DUPSORT);
  //   if (ret != 0) _dberr(ret);
  // }

  /* set key compare function if needed */
  // if (cmpfunc){
  //   ret = dbp->set_bt_compare(dbp, cmpfunc);
  //   if (ret != 0) _dberr(ret);
  // }


  /* Open the database */
  ret = dbp->open(dbp,           /* Pointer to the database */
                  NULL,          /* Txn pointer */
                  (fname + ".db").c_str(), /* File name */
                  NULL,          /* DB name */
                  DB_BTREE,      /* Database type (using btree) */
                  flags,         /* Open flags */
                  0600);            /* File mode. Using defaults */
  if (ret != 0) _dberr(ret);

  refcounter   = new int;
  *refcounter  = 1;
}

/* Open a secondary database, associated with some key
   in json objects of the primary database. */
void
JsonDB::open_sec(const std::string & key,
             const bool dup){

  /* Initialize the DB handle */
  DB * sdbp;
  int ret = db_create(&sdbp, NULL, 0);
  if (ret != 0) _dberr(ret, key);

  /* setup key duplication if needed */
  if (dup){
    ret = sdbp->set_flags(sdbp, DB_DUPSORT);
    if (ret != 0) _dberr(ret, key);
  }
  /* set key compare function if needed */
  // if (cmpfunc){
  //   ret = dbp->set_bt_compare(dbp, cmpfunc);
  //   if (ret != 0) _dberr(ret, key);
  // }
  /* Open the database */
  ret = sdbp->open(sdbp,         /* Pointer to the database */
                  NULL,          /* Txn pointer */
                  (fname + "." + key + ".db").c_str(), /* File name */
                  NULL,          /* DB name in the file */
                  DB_BTREE,      /* Database type (using btree) */
                  flags,         /* Open flags */
                  0600);         /* */
  if (ret != 0) _dberr(ret, key);

  ret = sdbp->associate(dbp, NULL, sdbp, jsonbd_key_extractor, 0);
  if (ret != 0) _dberr(ret, key);

  sec_dbp[key] = sdbp;
  jsondb_sec_keys[sdbp] = key; // put key in the global db index for the key extractor
}


/************************************/
// Non-json operations

// check if the database is empty
bool
JsonDB::is_empty(){
  DBC *curs;
  DBT key = mk_dbt();
  DBT val = mk_dbt();
  /* Get a cursor */
  dbp->cursor(dbp, NULL, &curs, 0);
  if (!curs) _dberr("can't get a cursor");
  /* Try to get the first value */
  int ret = curs->get(curs, &key, &val, DB_NEXT);
  curs->close(curs);
  if (ret == DB_NOTFOUND) return true;
  if (ret) _dberr("can't use a cursor");
  return false;
}

// put_str functions (integer or string key)
void
JsonDB::put_str(const jsondb_ikey_t ikey, const std::string & sval, const bool overwrite){
  if (!intkeys) _dberr("not an integer-key database");
  DBT key = mk_dbt(&ikey);
  DBT val = mk_dbt(sval);
  int ret = dbp->put(dbp, NULL, &key, &val, overwrite? 0:DB_NOOVERWRITE);
  if (ret!=0) _dberr(ret);
}
void
JsonDB::put_str(const std::string & skey, const std::string & sval, const bool overwrite){
  if (intkeys) _dberr("not a string-key database");
  DBT key = mk_dbt(skey);
  DBT val = mk_dbt(sval);
  int ret = dbp->put(dbp, NULL, &key, &val, overwrite? 0:DB_NOOVERWRITE);
  if (ret!=0) _dberr(ret);
}

// exists functions (integer or string key)
bool
JsonDB::exists(const jsondb_ikey_t ikey){
  if (!intkeys) _dberr("not an integer-key database");
  DBT key = mk_dbt(&ikey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret == DB_NOTFOUND) return false;
  if (ret) _dberr(ret);
  return true;
}
bool
JsonDB::exists(const std::string & skey){
  if (intkeys) _dberr("not a string-key database");
  DBT key = mk_dbt(skey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret == DB_NOTFOUND) return false;
  if (ret) _dberr(ret);
  return true;
}

// get_str functions (integer or string key)
std::string
JsonDB::get_str(const jsondb_ikey_t ikey){
  if (!intkeys) _dberr("not an integer-key database");
  DBT key = mk_dbt(&ikey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret != 0) _dberr(ret);
  return std::string((const char *)val.data);
}
std::string
JsonDB::get_str(const std::string & skey){
  if (intkeys) _dberr("not a string-key database");
  DBT key = mk_dbt(skey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret != 0) _dberr(ret);
  return std::string((const char *)val.data);
}

/************************************/
// for integer IDs we can get the last one and write the next one

jsondb_ikey_t
JsonDB::get_last_id(){
  if (!intkeys) _dberr("not an integer-key database");
  DBC *curs;
  dbp->cursor(dbp, NULL, &curs, 0);
  if (!curs) _dberr("can't get a cursor");
  DBT key = mk_dbt();
  DBT val = mk_dbt();
  int ret = curs->get(curs, &key, &val, DB_PREV);
  curs->close(curs);
  if (ret==DB_NOTFOUND) return 0;
  if (ret) _dberr(ret);
  return *(jsondb_ikey_t *)key.data;
}

/************************************/
// for integer IDs we can use time as a key (for log entries)

jsondb_ikey_t
JsonDB::put_str_time(const std::string & sval){
  if (!intkeys) _dberr("not an integer-key database");
  jsondb_ikey_t id = time(NULL)*1000;
  DBT key = mk_dbt(&id);
  DBT val = mk_dbt(sval);
  /* Try to put entry, if id exists, increase it: */
  int ret;
  while( (ret = dbp->put(dbp, NULL, &key, &val, DB_NOOVERWRITE)) == DB_KEYEXIST) id++;
  if (ret!=0) _dberr(ret);
  return id;
}

/************************************/
// get all the database entries as a json object
Json
JsonDB::get_all(){
  DBC *curs;
  DBT key = mk_dbt();
  DBT val = mk_dbt();
  /* Get a cursor */
  dbp->cursor(dbp, NULL, &curs, 0);
  if (!curs) _dberr("can't get a cursor");
  Json jj = Json::object();
  int ret;
  while ((ret=curs->get(curs, &key, &val, DB_NEXT))==0){
    std::ostringstream key_str;
    if (intkeys) key_str << *(jsondb_ikey_t *)key.data;
    else         key_str << (const char*)key.data;
    jj.set(key_str.str(), Json::load_string((const char *)val.data));
  }
  curs->close(curs);
  return jj;
}

// get entries with the specified key-value pair as a json object
Json
JsonDB::get_sec(const std::string & key, const std::string & val){

  // get secondary database for the key
  if (sec_dbp.count(key)==0)
    _dberr(std::string("no secondary database for the key: ") + key);
  DB *sdbp = sec_dbp[key];

  DBC *curs;
  DBT skey = mk_dbt(val);
  DBT pkey = mk_dbt();
  DBT pval = mk_dbt();
  /* Get a cursor */
  sdbp->cursor(sdbp, NULL, &curs, 0);
  if (!curs) _dberr("can't get a cursor");
  Json jj = Json::object();
  int ret;
  int fl = DB_SET;
  while ((ret=curs->c_pget(curs, &skey, &pkey, &pval, fl))==0){
    fl = DB_NEXT_DUP;
    std::ostringstream pkey_str;
    if (intkeys)  pkey_str << *(jsondb_ikey_t *)pkey.data;
    else          pkey_str << (const char*)pkey.data;
    jj.set(pkey_str.str(), Json::load_string((const char *)pval.data));
  }
  curs->close(curs);
  return jj;
}

/******************************************************************************/
int
jsonbd_key_extractor(DB *secdb, const DBT *pkey,
                     const DBT *pdata, DBT *skey){

  //  get key_name for the secondary DB:
  if (jsondb_sec_keys.find(secdb)==jsondb_sec_keys.end())
    throw JsonDB::Err() << "No key name for the secondary database";
  std::string key_name = jsondb_sec_keys.find(secdb)->second;

  // parse json, check that it is an object with key field
  const Json jj(Json::load_string((const char *)pdata->data));
  if (!jj.is_object()) throw JsonDB::Err() << "can't parse json";

  Json jv = jj.get(key_name);

  if (jv.is_null()){
    skey->flags = DB_DBT_MULTIPLE;
    skey->data = NULL;
    skey->size = 0;
    return 0;
  }

  // collect all values into set of strings
  // note: bercleydb wants only unique keys here
  std::set<std::string> v;
  if (jv.is_string()){
    v.insert(jv.as_string());
  }
  else if (jv.is_array()){
    for (unsigned int i=0; i<jv.size(); i++) v.insert(jv[i].as_string());
  }
  else throw JsonDB::Err() << "string or array expected: " << key_name;

  // build the DBT output
  int n = v.size();
  DBT * outdbt = (DBT*)malloc(sizeof(DBT) * n);
  memset(outdbt, 0, sizeof(DBT) * n);

  std::set<std::string>::const_iterator it;
  unsigned int i;
  for (it=v.begin(), i=0; it!=v.end(); it++, i++){
    /* we need to allocate memory for data! */
    char * str = (char *)malloc(it->length());
    if (!str) throw JsonDB::Err() << "malloc error";
    strcpy(str, it->c_str());
    // std::cerr << "Extractor: " << key_name << " = " << str << "\n";
    outdbt[i].data = str;
    outdbt[i].size = strlen(str)+1;
  }
  skey->flags = DB_DBT_MULTIPLE | DB_DBT_APPMALLOC;
  skey->data = outdbt;
  skey->size = n;
  return 0;
}

