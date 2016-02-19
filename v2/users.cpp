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
#include "act.h"
#include "jdb.h"

using namespace std;

/* check argument number */
void
check_args(const int argc, const int n){
  if (argc!=n+1)
    throw Err() << "wrong number of arguments, should be " << n;
}

/********************************************************************/
/** Actions
/********************************************************************/
void
do_login(const CFG & cfg, int argc, char **argv){

  Err("login");  // set error type
  check_args(argc, 0); // check argument number

  /* read login token from stdin*/
  string token;
  cin >> token;
  if (!cin.good()) throw Err() << "login token expected";

  json_t *root = get_login_info(cfg, token.c_str());

  std::string id = j_getstr(root, "identity");

  /* update user information from the database */
  UserDB user_db(cfg, DB_CREATE);
  json_t *user =  user_db.get_by_id(id);
  if (user){
    j_putstr(root, "level", j_getstr(user, "level"));
    j_putstr(root, "alias", j_getstr(user, "alias"));
    j_putstr(root, "abbr", j_getstr(user, "abbr"));
  }
  else{
    j_putstr(root, "level", user_db.is_empty()? "admin":"norm");
    j_putstr(root, "abbr", "");
  }
  json_decref(user);

  /* Create session */
  j_putstr(root, "session", user_db.make_session());

  /* Write user to the database */
  user_db.put(root);

  /* return user information */
  throw Exc() << j_dumpstr(root);
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
  Err("user_info");
  check_args(argc, 0);

  /* read session id from stdin*/
  string s;
  cin >> s;
  if (!cin.good()) throw Err() << "session id expected";
  UserDB user_db(cfg, DB_RDONLY);
  json_t * user = user_db.get_by_session(s);
  if (!user) throw Err() << "unknown session id";

  throw Exc() << j_dumpstr(user);
}

/********************************************************************/
void
do_set_alias(const CFG & cfg, int argc, char **argv){
  Err("user_info");
  check_args(argc, 1);
  const char *alias = argv[1];

  /* read session id from stdin*/
  string s;
  cin >> s;
  if (!cin.good()) throw Err() << "session id expected";
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
  Err("user_info");
  check_args(argc, 1);
  const char *abbr = argv[1];

  /* read session id from stdin*/
  string s;
  cin >> s;
  if (!cin.good()) throw Err() << "session id expected";
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
