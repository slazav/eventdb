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

   "id" field and duplication

*/
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
      std::string text()  const { return "jsondb error: " + s.str(); }
      std::string str()   const { return s.str(); }
  };

  // wrappers for throwing berkleydb errors from the class
  private:
    std::string cut_fname(const std::string &s) const{
      size_t n = s.rfind('/');
      return n==std::string::npos? s : s.substr(n+1,s.length()); }
    void _dberr(const std::string & s) const
      { throw Err() << cut_fname(fname) << ".db: " << s; }
    void _dberr(const int ret) const
      { throw Err() << cut_fname(fname) << ".db: " << db_strerror(ret); }
    void _dberr(const int ret, const std::string &key) const
      { throw Err() << cut_fname(fname) << "." << key << ".db: " << db_strerror(ret); }

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

  /************************************/
  // Non-json operations

  // check if the database is empty
  bool is_empty();

  // for integer IDs we can get the last one
  // (or 0 if database is empty).
  json_int_t get_last_id();

  // Put the json object into the database.
  // Integer or string key should be in "id" field of the object.
  // If integer id < 0 then the next available id is used
  // and the object is updated.
  void put(Json & jj, const bool overwrite = true);

  // Put the object with id equals to current unique time in ms.
  // "id" field in the object is not used, after run it is set to
  // the time.
  void put_time(Json & jj);

  // exists functions (integer or string key)
  bool exists(const json_int_t ikey) const;
  bool exists(const std::string & skey) const;

  // get functions (integer or string key)
  std::string get_str(const std::string & skey) const;
  std::string get_str(const json_int_t ikey) const;
  Json get(const std::string & skey) const;
  Json get(const json_int_t ikey) const;


  /************************************/
  // get all the database entries as a json array
  Json get_all();

  /************************************/
  /* Open a secondary database, associated with some key
     in json objects of the primary database. */
  void secondary_open(const std::string & key, const bool dup = false);

  // get entries with the specified key:value pair as a json array
  Json secondary_get(const std::string & key, const std::string & val);

  // check that there are any entries in the secondary db for key:value pair
  bool secondary_exists(const std::string & key, const std::string & val);

};

#endif
