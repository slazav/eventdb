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

/* Universal key extractor: extracts string or array of strings,
or "id" fields in objects/arrays of objects from a json object */
int jsonbd_key_extractor(DB *secdb, const DBT *pkey, const DBT *pdata, DBT *skey);

/********************************************************************/

// create DBT objects
DBT
mk_dbt(){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  return ret;
}
DBT
mk_dbt(const json_int_t * i){
  DBT ret = mk_dbt();
  ret.data = (void *)i;
  ret.size = sizeof(json_int_t);
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

// for integer IDs we can get the last one and write the next one
json_int_t
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
  return *(json_int_t *)key.data;
}

// put function
void
JsonDB::put(Json & jj, const bool overwrite){

  if (!jj["id"]) _dberr("no id field in the json");

  if (intkeys && !jj["id"].is_integer()) _dberr("integer id expected");
  if (!intkeys && !jj["id"].is_string()) _dberr("string id expected");

  json_int_t ikey; // we have to keep the data until dbp->put
  std::string skey;
  DBT key;
  if (intkeys){
    if (jj["id"].as_integer() < 0) jj.set("id", Json(get_last_id()+1));
    ikey = jj["id"].as_integer();
    key = mk_dbt(&ikey);
  }
  else {
    skey=jj["id"].as_string();
    key = mk_dbt(skey);
  }
  std::string sval = jj.save_string(JSON_PRESERVE_ORDER);
  DBT val = mk_dbt(sval.c_str());
  int ret = dbp->put(dbp, NULL, &key, &val, overwrite? 0:DB_NOOVERWRITE);
  if (ret!=0) _dberr(ret);
}

// for integer IDs we can use time as a key (for log entries)
void
JsonDB::put_time(Json & jj){
  if (!intkeys) _dberr("not an integer-key database");
  json_int_t t = time(NULL)*1000;
  while (exists(t)) t++;
  jj.set("id", t);
  put(jj, false);
}

// exists functions (integer or string key)
bool
JsonDB::exists(const json_int_t ikey) const{
  if (!intkeys) _dberr("not an integer-key database");
  DBT key = mk_dbt(&ikey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret == DB_NOTFOUND) return false;
  if (ret) _dberr(ret);
  return true;
}
bool
JsonDB::exists(const std::string & skey) const{
  if (intkeys) _dberr("not a string-key database");
  DBT key = mk_dbt(skey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret == DB_NOTFOUND) return false;
  if (ret) _dberr(ret);
  return true;
}

// get functions (integer or string key)
std::string
JsonDB::get_str(const json_int_t ikey) const{
  if (!intkeys) _dberr("not an integer-key database");
  DBT key = mk_dbt(&ikey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret != 0) _dberr(ret);
  return std::string((const char *)val.data);
}
std::string
JsonDB::get_str(const std::string & skey) const{
  if (intkeys) _dberr("not a string-key database");
  DBT key = mk_dbt(skey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret != 0) _dberr(ret);
  return std::string((const char *)val.data);
}
Json
JsonDB::get(const json_int_t ikey) const{
  if (!intkeys) _dberr("not an integer-key database");
  DBT key = mk_dbt(&ikey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret != 0) _dberr(ret);
  return Json::load_string((const char *)val.data);
}
Json
JsonDB::get(const std::string & skey) const{
  if (intkeys) _dberr("not a string-key database");
  DBT key = mk_dbt(skey);
  DBT val = mk_dbt();
  int ret = dbp->get(dbp, NULL, &key, &val, 0);
  if (ret != 0) _dberr(ret);
  return Json::load_string((const char *)val.data);
}

// delete functions
void
JsonDB::del(const json_int_t ikey){
  if (!intkeys) _dberr("not an integer-key database");
  DBT key = mk_dbt(&ikey);
  int ret = dbp->del(dbp, NULL, &key, 0);
  if (ret != 0) _dberr(ret);
}
void
JsonDB::del(const std::string & skey){
  if (intkeys) _dberr("not a string-key database");
  DBT key = mk_dbt(skey);
  int ret = dbp->del(dbp, NULL, &key, 0);
  if (ret != 0) _dberr(ret);
}

/********************************************************************/

// get all the database entries as a json array
Json
JsonDB::get_all(const std::string & key){
  DBC *curs;
  DBT skey = mk_dbt();
  DBT pkey = mk_dbt();
  DBT pval = mk_dbt();
  Json jj = Json::array();

  if (key == ""){
    // Get a cursor
    dbp->cursor(dbp, NULL, &curs, 0);
    if (!curs) _dberr("can't get a cursor");
    // Extract all entries
    while (curs->get(curs, &pkey, &pval, DB_NEXT)==0)
      jj.append(Json::load_string((const char *)pval.data));
    curs->close(curs);
  }
  else {
    // get secondary database for the key
    if (sec_dbp.count(key)==0)
      _dberr(std::string("no secondary database for the key: ") + key);
    // Get a cursor
    sec_dbp[key]->cursor(sec_dbp[key], NULL, &curs, 0);
    if (!curs) _dberr("can't get a cursor");

    while (curs->c_pget(curs, &skey, &pkey, &pval, DB_NEXT)==0){
      Json o=Json::object();
      o.set("skey", (const char *)skey.data);
      if (intkeys) o.set("pkey", *(json_int_t *)pkey.data);
      else         o.set("pkey", (const char *)pkey.data);
      jj.append(o);
    }
    curs->close(curs);
  }
  return jj;
}

/********************************************************************/

/* Open a secondary database, associated with some key
   in json objects of the primary database. */
void
JsonDB::secondary_open(const std::string & key, const bool dup){

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

// get entries with the specified key-value pair as a json array
Json
JsonDB::secondary_get(const std::string & key, const std::string & val){

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
  Json jj = Json::array();
  int ret;
  int fl = DB_SET;
  while ((ret=curs->c_pget(curs, &skey, &pkey, &pval, fl))==0){
    fl = DB_NEXT_DUP;
    jj.append(Json::load_string((const char *)pval.data));
  }
  curs->close(curs);
  return jj;
}

// check that there are any entries in the secondary db for key:value pair
bool
JsonDB::secondary_exists(const std::string & key, const std::string & val){
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
  ret=curs->c_pget(curs, &skey, &pkey, &pval, DB_SET);
  curs->close(curs);
  return ret==0;
}

/******************************************************************************/

int
jsonbd_key_extractor(DB *secdb, const DBT *pkey,
                     const DBT *pdata, DBT *skey){

  //  get key_name for the secondary DB:
  if (jsondb_sec_keys.find(secdb)==jsondb_sec_keys.end())
    throw JsonDB::Err() << "No key name for the secondary database";
  std::string key_name = jsondb_sec_keys.find(secdb)->second;

  // parse json, check that there is an object with key field
  const Json jj(Json::load_string((const char *)pdata->data));
  if (!jj.is_object()) throw JsonDB::Err() << "can't parse json";

  Json jv = jj.get(key_name);

  // null object: extract no entries
  if (jv.is_null()){
    skey->flags = DB_DBT_MULTIPLE;
    skey->data = NULL;
    skey->size = 0;
    return 0;
  }

  // collect all values into set of strings
  // note: bercleydb wants only unique keys here
  std::set<std::string> v;

  // string:
  if (jv.is_string()){
    v.insert(jv.as_string());
  }
  // object with "id" field:
  else if (jv.is_object() && jv.exists("id")
           && jv.get("id").is_string()){
    v.insert(jv.get("id").as_string());
  }
  // array:
  else if (jv.is_array()){
    for (unsigned int i=0; i<jv.size(); i++){
      // string in the array element
      if (jv[i].is_string())
        v.insert(jv[i].as_string());
      // object with "id" field in the array element
      else if (jv[i].is_object() && jv[i].exists("id")
               && jv[i].get("id").is_string())
        v.insert(jv[i].get("id").as_string());
    }
  }
  else throw JsonDB::Err() << "strings or objects with id field expected: " << key_name;

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

