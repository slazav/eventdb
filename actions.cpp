#include <iostream>

#include <cstdio> /* sprintf */
#include <cstring> /* memcpy, memset */
#include <unistd.h> /* read, write, close */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <openssl/md5.h>
#include "login.h"
#include "actions.h"

using namespace std;

/* check argument number */
void
check_args(const int argc, const int n){
  if (argc!=n+1)
    throw Err() << "wrong number of arguments, should be " << n;
}
/* get secret (login token or session id) from stdin */
string
get_secret(){
  string s;
  cin >> s;
  if (!cin.good()) throw Err() << "can't get secret";
  return s;
}

/* create random session id */
std::string make_session(){
  std::string session(25, ' ');
  static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (int i = 0; i < session.length(); ++i)
  session[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  return session;
}


/********************************************************************/
// user database
//  identity -> (name, alias, abbr, session, stime)
//  secondary: sessions aliases

class UserDB : public JsonDB{
  UserDB(const CFG & cfg, const int flags=0): JsonDB(cfg.datadir + "/user", flags){
    open_sec("session", false);
    open_sec("alias", false);
  }
  json::Value get_by_session(const std::string & sess, std::string & id){
    if (sess == ""){
      json::Value ret(json::object());
      ret.set_key("identity", "");
      ret.set_key("name",     "");
      ret.set_key("alias",    "");
      ret.set_key("level",    "anon");
      ret.set_key("session",  "");
      ret.set_key("stime",    0);
      return ret;
    }
    json::Value ret = get_sec("session", sess);
    json::Iterator i(ret);
    if (!i.valid()) throw Err() << "login error";
    id = ret.key();
    return ret.value().as_string();
  }
};

/********************************************************************/
/** Actions
/********************************************************************/
void
do_login(const CFG & cfg, int argc, char **argv){

  Err("login");                // set error type
  check_args(argc, 0);         // check argument number
  string token = get_secret(); // read login token from stdin

  /* get user login information (from loginza of cfg file) */
  json::Value *userl = get_login_info(cfg, token.c_str());

  /* extract user identity */
  string id = JsonDB::json_getstr(userl, "identity");

  /* update user information from the database */
  UserDB udb(cfg, DB_CREATE);

  if (udb.exists(id)){
    /* known user: get level, alias, abbr from DB */
    json::Value userd = udb.get(id);
    userl.set_key("level", userd.getv("level"));
    userl.set_key("alias", userd.getv("alias"));
    userl.set_key("abbr",  userd.getv("abbr"));
  }
  else{
    /* new user: for the very first user level="admin" */
    userl.set_key("level", udb.is_empty()? "admin":"norm");
    userl.set_key("abbr", "");
  }

  /* Create new session */
  userl.set_key("session", json::Value(make_session()));
  userl.set_key("stime",   json::Value(time(NULL)));

  /* Write user to the database */
  udb.put(id, userl);

  /* return user information */
  throw Exc() << userl.save_string();
}

/********************************************************************/
void
do_logout(const CFG & cfg, int argc, char **argv){
  Err("logout");
  check_args(argc, 0);

  /* read session id from stdin*/
  UserDB udb(cfg);
  std::string id;
  json::Value user = udb.get_by_session(get_secret(), id);

  /* remove session */
  user.set_key("session", "");
  user.set_key("stime",    0);
  /* Write user to the database */
  udb.put(user);

  throw Exc() << j_dumpstr(user);
}

/********************************************************************/
void
do_user_info(const CFG & cfg, int argc, char **argv){

  Err("user_info");        // set error type
  check_args(argc, 0);     // check argument number
  string s = get_secret(); // read session id from stdin

  /* get user information from DB and dump it to stdout */
  UserDB user_db(cfg, DB_RDONLY);
  json_t * user = user_db.get_by_session(s);
  if (!user) throw Err() << "unknown session id";
  throw Exc() << j_dumpstr(user);
}

/********************************************************************/
void
do_set_alias(const CFG & cfg, int argc, char **argv){
  Err("set_alias");
  check_args(argc, 1);
  const char *alias = argv[1];
  string s = get_secret();

  UserDB user_db(cfg);
  json_t * user = user_db.get_by_session(s);
  if (!user) throw Err() << "unknown session id";

  /* change alias */
  j_putstr(user, "alias", alias);
  /* Write user to the database */
  user_db.put(user);

  throw Exc() << j_dumpstr(user);
}

/********************************************************************/
void
do_set_abbr(const CFG & cfg, int argc, char **argv){
  Err("set_abbr");
  check_args(argc, 1);
  const char *abbr = argv[1];
  string s = get_secret();

  UserDB user_db(cfg);
  json_t * user = user_db.get_by_session(s);
  if (!user) throw Err() << "unknown session id";

  /* change alias */
  j_putstr(user, "abbr", abbr);
  /* Write user to the database */
  user_db.put(user);

  throw Exc() << j_dumpstr(user);
}

/********************************************************************/
