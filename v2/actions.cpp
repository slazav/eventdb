#include <iostream>

#include <cstdio> /* sprintf */
#include <cstring> /* memcpy, memset */
#include <unistd.h> /* read, write, close */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <openssl/md5.h>
#include "json.h"
#include "login.h"
#include "jdb.h"
#include "actions.h"

using namespace std;

/* check argument number */
void
check_args(const int argc, const int n){
  if (argc!=n+1)
    throw Err() << "wrong number of arguments, should be " << n;
}
/* get secret (login token or session id) from stdin */
std::string
get_secret(){
  string s;
  cin >> s;
  if (!cin.good()) throw Err() << "can't get secret";
  return s;
}


/********************************************************************/
/** Actions
/********************************************************************/
void
do_login(const CFG & cfg, int argc, char **argv){

  Err("login");                // set error type
  check_args(argc, 0);         // check argument number
  string token = get_secret(); // read login token from stdin

  /* get user login information (from loginza of cfg file) */
  json_t *userl = get_login_info(cfg, token.c_str());

  /* extract user identity */
  std::string id = j_getstr(userl, "identity");

  /* update user information from the database */
  UserDB user_db(cfg, DB_CREATE);
  json_t *userdb =  user_db.get_by_id(id);
  if (userdb){
    /* known user: get level, alias, abbr from DB */
    j_putstr(userl, "level", j_getstr(userdb, "level"));
    j_putstr(userl, "alias", j_getstr(userdb, "alias"));
    j_putstr(userl, "abbr",  j_getstr(userdb, "abbr"));
  }
  else{
    /* new user: for the very first user level="admin" */
    j_putstr(userl, "level", user_db.is_empty()? "admin":"norm");
    j_putstr(userl, "abbr", "");
  }
  json_decref(userdb);

  /* Create new session */
  j_putstr(userl, "session", user_db.make_session());

  /* Write user to the database */
  user_db.put(userl);

  /* return user information */
  throw Exc() << j_dumpstr(userl);
}



/********************************************************************/
void
do_logout(const CFG & cfg, int argc, char **argv){
  Err("logout");
  check_args(argc, 0);

  /* read session id from stdin*/
  string s;
  cin >> s;
  if (!cin.good()) throw Err() << "session id expected";
  UserDB user_db(cfg);
  json_t * user = user_db.get_by_session(s);
  if (!user) throw Err() << "unknown session id";

  /* remove session */
  j_putstr(user, "session", "");
  /* Write user to the database */
  user_db.put(user);

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
