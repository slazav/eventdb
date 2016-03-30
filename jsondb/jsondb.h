#ifndef JDB_H
#define JDB_H

#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <db.h>
#include "../jsonxx/jsonxx.h" // my wrapper for jansson library

/********************************************************************/

/* BercleyDB with json support

  libraries:
    bercleydb: http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
               https://web.stanford.edu/class/cs276a/projects/docs/berkeleydb/reftoc.html
    jansson:   https://github.com/akheron/jansson
               http://jansson.readthedocs.org/en/latest/

   Keys are unique integers or strings, values are json strings.
   Secondary indices are supported to look through fields in json objects.

*/
/********************************************************************/

// Type for integer keys. We need > 64bit for ms time in logs
typedef unsigned long long int jsondb_ikey_t;

/********************************************************************/
// The main JsonDB class

class JsonDB{

  /************************************/
  // Error class for exceptions
  public:
  class Err {
    std::ostringstream s;
    public:
      Err(){}
      Err(const Err & o) { s << o.s.str(); }
      template <typename T>
      Err & operator<<(const T & o){ s << o; return *this; }
      std::string json() const {
        return std::string("{\"error_type\": \"jsondb\", ") +
                            "\"error_message\":\"" + s.str() + "\"}"; }
      std::string str() const { return s.str(); }
  };

  // wrappers for throwing berkleydb errors from the class
  private:
    void _dberr(const std::string & s) const
      { throw Err() << fname << ": " << s; }
    void _dberr(const int ret) const
      { throw Err() << fname << ": " << db_strerror(ret); }
    void _dberr(const int ret, const std::string &key) const
      { throw Err() << fname << ", secondary key \"" << key << "\""
                    << ": " << db_strerror(ret); }


  /************************************/
  // database handles, refcounter and memory menegement.
  private:
    DB  *dbp;          /* berkleydb handle    */
    std::string fname; /* database filename   */
    int flags;         /* database open flags */
    bool intkeys;      /* integer/string keys */
    std::map<std::string, DB*> sec_dbp; /* secondary db handles */
    int * refcounter;

    void copy(const JsonDB & other);
    void destroy(void);

  /************************************/
  /* Copy constructor, destructor, assignment */
  public:

    JsonDB(const JsonDB & other){ copy(other); }

    JsonDB & operator=(const JsonDB & other){
      if (this != &other){ destroy(); copy(other); }
      return *this;
    }

    ~JsonDB(){ destroy(); }

  /************************************/
  /* Main constructor
       file    -- database filename
       flags   -- DB_CREATE | DB_EXCL | DB_RDONLY | DB_TRUNCATE ...
       intkeys -- db use integer/string keys
   */
  JsonDB(const std::string & fname_,
         const bool intkeys_ = true,
         const int flags_=0);

  /* Open a secondary database, associated with some key
     in json objects of the primary database. */
  void open_sec(const std::string & key, const bool dup = false);

  /************************************/
  // Non-json operations

  // check if the database is empty
  bool is_empty();

  // put_str functions (integer or string key)
  void put_str(const jsondb_ikey_t ikey,
               const std::string & sval, const bool overwrite = true);
  void put_str(const std::string & skey,
               const std::string & sval, const bool overwrite = true);

  // exists functions (integer or string key)
  bool exists(const jsondb_ikey_t ikey);
  bool exists(const std::string & skey);

  // get_str functions (integer or string key)
  std::string get_str(const jsondb_ikey_t ikey);
  std::string get_str(const std::string & skey);

  /************************************/
  // json operations

  // put_json functions (integer or string key)
  void put_json(const jsondb_ikey_t ikey, const Json & jj, const bool overwrite = true){
    put_str(ikey, jj.save_string(JSON_PRESERVE_ORDER), overwrite); }
  void put_json(const std::string & skey, const Json & jj, const bool overwrite = true){
    put_str(skey, jj.save_string(JSON_PRESERVE_ORDER), overwrite); }

  // get_json functions (integer or string key)
  Json get_json(const std::string & skey){
    return Json::load_string(get_str(skey)); }
  Json get_json(const jsondb_ikey_t ikey){
    return Json::load_string(get_str(ikey)); }

  /************************************/
  // for integer IDs we can get the last one and write the next one

  jsondb_ikey_t get_last_id();
  void put_str_next(const std::string & sval, const bool overwrite = true){
    put_str(get_last_id()+1, sval, overwrite); }
  void put_json_next(const Json & jj, const bool overwrite = true){
    put_json(get_last_id()+1, jj, overwrite); }

  /************************************/
  // for integer IDs we can use time as a key (for log entries)

  jsondb_ikey_t put_str_time(const std::string & sval);
  jsondb_ikey_t put_json_time(const Json & jj){
    return put_str_time(jj.save_string(JSON_PRESERVE_ORDER));}

  /************************************/
  // get all the database entries as a json object
  Json get_all();

  // get entries with the specified key-value pair as a json object
  Json get_sec(const std::string & key, const std::string & val);

};


#endif
