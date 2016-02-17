#ifndef DBS_H
#define DBS_H

#include <string>
#include <cstring> /* memset */
#include <db.h>
#include "err.h"
#include <jansson.h>

/* Database base class */
/* see http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
 * for bercleydb docs and examples */

/* keys and values are strings */

class Database{
  DB *dbp;  /* berkleydb handle */
  std::string name;

  typedef int (*cmpfunc_t)(DB *db, const DBT *dbt1,
                const DBT *dbt2);  /* comparison function */

public:
  Database(){ dbp=NULL;}
  ~Database(){ close(); }

  /* open/close database */
  void open(const std::string & file, const int flags, bool dup=false, cmpfunc_t cmpfunc=NULL){
    int ret;
    name = file;

    /* Check if database already opend */
    if (dbp) throw Err("db_create") << "Database already opened";

    /* Initialize the DB handle */
    ret = db_create(&dbp, NULL, 0);
    if (ret != 0) throw Err("db_create") << db_strerror(ret);

    /* setup key duplication if needed */
    if (dup){
      ret = dbp->set_flags(dbp, DB_DUPSORT);
      if (ret != 0) throw Err("db_set_flags") << db_strerror(ret);
    }

    /* set key compare function if needed */
    if (cmpfunc){
      ret = dbp->set_bt_compare(dbp, cmpfunc);
      if (ret != 0) throw Err("db_set_bt_compare") << db_strerror(ret);
    }

    /* Now open the database */
    ret = dbp->open(dbp,         /* Pointer to the database */
                    NULL,        /* Txn pointer */
                    file.c_str(), /* File name */
                    NULL,        /* Logical db name (unneeded) */
                    DB_BTREE,    /* Database type (using btree) */
                    flags,       /* Open flags */
                    0);          /* File mode. Using defaults */
    if (ret != 0) throw Err("db_open") << db_strerror(ret);
  }

  void close(){
    int ret;
    name="";
    if (dbp && (ret=dbp->close(dbp, 0))!=0)
      throw Err("db_close") << db_strerror(ret);
    dbp=NULL;
  }


  /* create DBT objects */
  DBT mk_dbt(){
    DBT ret;
    memset(&ret, 0, sizeof(DBT));
    return ret;
  }
  DBT mk_dbt(const char * str){
    DBT ret;
    memset(&ret, 0, sizeof(DBT));
    ret.data = (char *) str;
    ret.size = strlen(str)+1;
    return ret;
  }
  DBT mk_dbt(const std::string & str){
    DBT ret;
    memset(&ret, 0, sizeof(DBT));
    ret.data = (char *) str.c_str();
    ret.size = str.length()+1;
    return ret;
  }

  /* exists/get/put */
  bool exists(const std::string & skey){
    int ret;
    DBT key = mk_dbt(skey);
    DBT val = mk_dbt();
    ret = dbp->get(dbp, NULL, &key, &val, 0);
    if (ret == DB_NOTFOUND) return false;
    if (ret == 0) return true;
    throw Err("db_set") << name <<  "database: "<< db_strerror(ret);
  }
  std::string get(const std::string & skey){
    int ret;
    DBT key = mk_dbt(skey);
    DBT val = mk_dbt();
    ret = dbp->get(dbp, NULL, &key, &val, 0);
    if (ret != 0) throw Err("db_get") << name << " database: " << db_strerror(ret);
    return std::string((const char *)val.data);
  }
  void put(const std::string & skey, const std::string & sval, bool overwrite){
    DBT key = mk_dbt(skey);
    DBT val = mk_dbt(sval);
    int ret = dbp->put(dbp, NULL, &key, &val, overwrite? 0:DB_NOOVERWRITE);
    if (ret!=0) throw Err("db_set") << name <<  "database: "<< db_strerror(ret);
  }

  /* get/put json */
  json_t * get_json(const std::string & skey){
    std::string sval = get(skey);
    json_error_t e;
    json_t * ret = json_loads(sval.c_str(), 0, &e);
    if (!ret) throw Err("db_get_json") << e.text;
    return ret;
  }
  void put_json(const std::string & skey, json_t * json, bool overwrite){
    char *str = json_dumps(json, 0);
    if (!str) throw Err("db_set_json") << "can't write json";
    std::string sval(str);
    free(str);
    put(skey, sval, overwrite);
  }


};

#endif