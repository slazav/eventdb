#ifndef JDB_H
#define JDB_H

#include <string>
#include <iostream>
#include <map>
#include <cstring> /* memset */
#include <db.h>
#include "err.h"
#include "cfg.h"
#include "json.h"

/* BercleyDB with json support
 *
 * see http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
 *     https://web.stanford.edu/class/cs276a/projects/docs/berkeleydb/reftoc.html
 * for bercleydb docs and examples.
 *
 * Keys are strings, values are json strings.
 */

/********************************************************************/
/* create DBT objects */
DBT mk_dbt();
DBT mk_dbt(const char * str);
DBT mk_dbt(const std::string & str);

/********************************************************************/
/* I do not know, how to keep this in the class... */
extern std::map<DB*, std::string> key_names;

/* get key_name*/
std::string jbd_get_keyname(DB* dbp);

/* secondary key extractor */
int jbd_key_extractor (DB *secdb, const DBT *pkey, const DBT *pdata, DBT *skey);

/********************************************************************/

class JDB{
public:
  DB *dbp;  /* berkleydb handle  */
  DB *pdbp; /* primary DB if any */
  std::string name;

  typedef int (*cmpfunc_t)(DB *db, const DBT *dbt1,
                const DBT *dbt2);  /* comparison function */

  /* open/close database */
  JDB(const std::string & file,
           const std::string & key_name,
           const int flags, bool dup=false,
           cmpfunc_t cmpfunc=NULL);
  ~JDB();

  /* Associate the database with another one.
     Use key_name parameter for both  databases for the association. */
  void associate(const JDB & prim_db);

  /* is the key exists? */
  bool exists(const std::string & skey);

  /* Put information into the database.
     Database key comes from json. */
  void put(json_t * json, bool overwrite);

  /* Get information from the database.
     For secondary databases primary key is added to the output.
     Remove it before putting information back to primary DB */
  json_t *get(const std::string & skey);

  /* is the database empty?*/
  bool is_empty();

};

/********************************************************************/
/* Databases for users:
 *  users (primary), identity -> (name, alias, abbr, session, stime)
 *  sessions (secondary)
 *  aliases  (secondary)
 *
 *  flags: DB_RDONLY, DC_CREATE
 */


class UserDB{
  JDB users, aliases, sessions;

public:
  /* constructor: open and associate databases */
  UserDB(const CFG & cfg, const int flags=0):
          users(cfg.datadir + "/users.db",       "identity", flags),
          sessions(cfg.datadir + "/sessions.db", "session",  flags),
          aliases(cfg.datadir + "/aliases.db",   "alias",    flags){
    /* associate secondary databases with the primary one */
    sessions.associate(users);
    aliases.associate(users);
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
    return users.get(identity);
  }

  json_t * get_by_session(const std::string &session){
    if (session == "") return anon_user();
    if (!sessions.exists(session)) return NULL;
    return sessions.get(session);
  }

  json_t * get_by_alias(const std::string &alias){
    if (!aliases.exists(alias)) return NULL;
    return aliases.get(alias);
  }

  void put(json_t * root){
    users.put(root, true);}

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