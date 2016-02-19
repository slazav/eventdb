#ifndef DBS_H
#define DBS_H

#include <string>
#include <cstring> /* memset */
#include <db.h>
#include "err.h"
#include "cfg.h"
#include "json.h"

/* Database classes
 * see http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
 * for bercleydb docs and examples.
 * Here keys are strings, values are json strings.
 */

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
mk_dbt(const std::string & str){
  DBT ret;
  memset(&ret, 0, sizeof(DBT));
  ret.data = (char *) str.c_str();
  ret.size = str.length()+1;
  return ret;
}

/********************************************************************/
/* Extract string value from the JSON object in pdata and put it into skey.
 * This function is used for secondary  database extractors.
 */
void
extract_field(const char *key, const DBT *pdata, DBT *skey){
  json_error_t e;
  json_t * root = json_loads((const char *)pdata->data, 0, &e);
  if (!root) throw Err() << e.text;
  json_t * val = json_object_get(root, key);
  if (!json_is_string(val)){
    json_decref(root);
    throw Err() << "can't get json value";
  }
  *skey = mk_dbt(json_string_value(val));
}

/********************************************************************/
/* base class */

class Database{
public:
  DB *dbp;  /* berkleydb handle */
  std::string name;

  typedef int (*cmpfunc_t)(DB *db, const DBT *dbt1,
                const DBT *dbt2);  /* comparison function */

  /* open/close database */
  Database(const std::string & file, const int flags, bool dup=false, cmpfunc_t cmpfunc=NULL){
    int ret;
    name = file;

    /* Initialize the DB handle */
    ret = db_create(&dbp, NULL, 0);
    if (ret != 0) throw Err() << db_strerror(ret);

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
    ret = dbp->open(dbp,         /* Pointer to the database */
                    NULL,        /* Txn pointer */
                    file.c_str(), /* File name */
                    NULL,        /* Logical db name (unneeded) */
                    DB_BTREE,    /* Database type (using btree) */
                    flags,       /* Open flags */
                    0);          /* File mode. Using defaults */
    if (ret != 0) throw Err() << db_strerror(ret);
  }

  ~Database(){
    int ret;
    name="";
    if (dbp && (ret=dbp->close(dbp, 0))!=0)
      throw Err() << db_strerror(ret);
    dbp=NULL;
  }


  /* exists/get/put */
  bool exists(const std::string & skey){
    int ret;
    DBT key = mk_dbt(skey);
    DBT val = mk_dbt();
    ret = dbp->get(dbp, NULL, &key, &val, 0);
    if (ret == DB_NOTFOUND) return false;
    if (ret == 0) return true;
    throw Err() << name <<  "database: "<< db_strerror(ret);
  }
  void put(const std::string & skey, const std::string & sval, bool overwrite){
    DBT key = mk_dbt(skey);
    DBT val = mk_dbt(sval);
    int ret = dbp->put(dbp, NULL, &key, &val, overwrite? 0:DB_NOOVERWRITE);
    if (ret!=0) throw Err() << name <<  "database: "<< db_strerror(ret);
  }
  std::string get(const std::string & skey){
    int ret;
    DBT key = mk_dbt(skey);
    DBT val = mk_dbt();
    ret = dbp->get(dbp, NULL, &key, &val, 0);
    if (ret != 0) throw Err() << name << " database: " << db_strerror(ret);
    return std::string((const char *)val.data);
  }
  // This is needed for secondary databeses, it gets key and value for the primary one.
  std::string pget(const std::string & skey, std::string &spkey){
    int ret;
    DBT key  = mk_dbt(skey);
    DBT pkey = mk_dbt();
    DBT pval = mk_dbt();
    ret = dbp->pget(dbp, NULL, &key, &pkey, &pval, 0);
    if (ret != 0) throw Err() << name << " database: " << db_strerror(ret);
    spkey = std::string((const char *)pkey.data);
    return std::string((const char *)pval.data);
  }

  /* get/put json */
  json_t * get_json(const std::string & skey){
    std::string sval = get(skey);
    json_error_t e;
    json_t * root = json_loads(sval.c_str(), 0, &e);
    if (!root) throw Err() << e.text;
    return root;
  }
  void put_json(const std::string & skey, json_t * json, bool overwrite){
    char *str = json_dumps(json, 0);
    if (!str) throw Err() << "can't write json";
    std::string sval(str);
    free(str);
    put(skey, sval, overwrite);
  }
  /*This is needed for secondary databeses, it gets key and value for the primary one
    and insert into the json pkey_name field*/
  json_t * pget_json(const std::string & skey, const char *pkey_name){
    std::string spkey;
    std::string sval = pget(skey, spkey);
    json_error_t e;
    json_t * root = json_loads(sval.c_str(), 0, &e);
    if (!root) throw Err() << e.text;
    json_object_set_new(root, pkey_name, json_string(spkey.c_str()));
    return root;
  }

  /* is the database empty?*/
  bool is_empty(){
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

};

/********************************************************************/
/* Databases for users:
 *  users (primary), identity -> (name, alias, abbr, session, stime)
 *  sessions (secondary)
 *  aliases  (secondary)
 */

/* extract sessions and aliases for secondary db */
int get_session(DB *secdb, const DBT *pkey, const DBT *pdata, DBT *skey){
  extract_field("session", pdata, skey); }
int get_alias(DB *secdb, const DBT *pkey, const DBT *pdata, DBT *skey){
  extract_field("alias", pdata, skey); }


class UserDB{
  Database users, aliases, sessions;


public:
  /* constructor: open and associate databases */
  UserDB(const CFG & cfg):
          users(cfg.datadir + "/users.db",    DB_CREATE),
          sessions(cfg.datadir + "/sessions.db", DB_CREATE),
          aliases(cfg.datadir + "/aliases.db",  DB_CREATE){
    /* associate secondary databases with the primary one */
    sessions.dbp->associate(users.dbp, NULL, sessions.dbp, get_session, 0);
    aliases.dbp->associate(users.dbp,  NULL, aliases.dbp,  get_alias,   0);
  }

  /* json information for an anonimous user */
  json_t * anon_user(){
    json_error_t e;
    json_t * root = json_pack_ex(&e, 0, "{ssssssssssss}",
      "identity", "", "name", "", "alias", "", "abbr", "", "level", "anon", "session", "");
    if (!root){ json_decref(root); throw Err("write_user") << e.text; }
    return root;
  }

  /* get user by identity, alias, session */
  json_t * get_by_id(const std::string &identity){
    if (!users.exists(identity)) return NULL;
    return users.get_json(identity);
  }

  json_t * get_by_session(const std::string &session){
    if (session == "") return anon_user();
    if (!sessions.exists(session)) return NULL;
    return sessions.pget_json(session, "identity");
  }

  json_t * get_by_alias(const std::string &alias){
    if (!aliases.exists(alias)) return NULL;
    return aliases.pget_json(alias, "identity");
  }

  void put(const char *id, json_t * root){
    users.put_json(id, root, true);}

  /* create random session id */
  std::string make_session(){
    std::string session(25, ' ');
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < session.length(); ++i)
    session[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    return session;
  }

  bool is_empty(){ return users.is_empty();}

};



#endif