#include <iostream>

#include <cstdio> /* sprintf, fgets */
#include <cstring> /* memcpy, memset */
#include <unistd.h> /* read, write, close */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <openssl/md5.h>
#include "login.h"
#include "actions.h"


#include "jsondb/jsondb.h"


using namespace std;


/********************************************************************/
// common functions

// check argument number, throw exception if it is wrong
void check_args(const int argc, const int n){
  if (argc!=n+1)
    throw Err() << "wrong number of arguments, should be " << n;
}

// secret buffer
char sec_buf[1024];

// fill secret buffer with zeros
void clr_secret(){ memset(sec_buf, 0, sizeof(sec_buf)); }

// get secret (login token or session id) from stdin
// we want to use static buffer for the secret and clear it after use
// (not good to use std::string for secrets)
char * get_secret(){
  if (fgets(sec_buf, sizeof(sec_buf)-1, stdin) == NULL){
    clr_secret();
    return sec_buf;
  }
  if (strlen(sec_buf)>0 && sec_buf[strlen(sec_buf)-1] == '\n')
    sec_buf[strlen(sec_buf)-1] = '\0'; // cut trailing '\n'
  return sec_buf;
}

// create random session id
std::string make_session(){
  std::string session(25, ' ');
  static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (int i = 0; i < session.length(); ++i){
    #ifdef __FreeBSD__
    session[i] = alphanum[arc4random_uniform(sizeof(alphanum) - 1)];
    #else
    session[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    #endif
  }
  return session;
}

/********************************************************************/
// user database
//  identity -> (name, alias, session, stime)
//  secondary: sessions aliases

class UserDB : public JsonDB{
  public:
  UserDB(const CFG & cfg, const int flags=0):
       JsonDB(cfg.datadir + "/user", false, flags){
    open_sec("session", false);
    open_sec("alias", false);
  }
  Json get_by_session(const std::string & sess, std::string & id){
    Json ret = get_sec("session", sess);
    if (ret.size()!=1) return Json::null();
    Json::iterator i = ret.begin();
    id = i.key();
    return i.val();
  }
};

Json
get_anon(){
  Json ret = Json::object();
  ret.set("identity", "anon");
  ret.set("alias",    "anon");
  ret.set("level",    "anon");
  ret.set("session",  "");
  ret.set("stime",    0);
  return ret;
}

/********************************************************************/
/** Actions
/********************************************************************/
void
do_login(const CFG & cfg, int argc, char **argv){

  Err("login");                // set error type
  check_args(argc, 0);         // check argument number

  /* get user login information (from loginza of cfg file) */
  Json userl = get_login_info(cfg, get_secret());
  clr_secret();

  /* extract user identity */
  string id = userl["identity"].as_string();
  if (id=="") throw Err() << "login error";

  /* update user information from the database */
  UserDB udb(cfg, DB_CREATE);
  if (udb.exists(id)){
    /* known user: update level and alias from DB */
    Json userd = udb.get_json(id);
    userl.set("level", userd["level"].as_string());
    userl.set("alias", userd["alias"].as_string());
  }
  else{
    /* new user: for the very first user level="admin" */
    userl.set("level", udb.is_empty()? "admin":"norm");
  }

  /* Create new session */
  userl.set("session", Json(make_session()));
  userl.set("stime",   Json((json_int_t)time(NULL)));

  /* Write user to the database */
  udb.put_json(id, userl);

  /* return user information */
  throw Exc() << userl.save_string(JSON_PRESERVE_ORDER);
}

/********************************************************************/
void
do_my_info(const CFG & cfg, int argc, char **argv){

  Err("my_info");          // set error type
  check_args(argc, 0);     // check argument number

  // Here an empty session is not an error.
  // In this case there is no need for opening db,
  // We just return an anonimous user
  char *sess = get_secret();
  if (strlen(sess)==0)
    throw Exc() << get_anon().save_string(JSON_PRESERVE_ORDER);

  UserDB udb(cfg);
  std::string id;
  Json user = udb.get_by_session(sess, id);
  clr_secret();
  if (!user) throw Err() << "authentication error";

  throw Exc() << user.save_string(JSON_PRESERVE_ORDER);
}

/********************************************************************/
void
do_logout(const CFG & cfg, int argc, char **argv){

  Err("logout");
  check_args(argc, 0);

  /* Get user information. Empty session is an error */
  UserDB udb(cfg);
  std::string id;
  Json user = udb.get_by_session(get_secret(), id);
  clr_secret();
  if (!user) throw Err() << "authentication error";

  user.del("session"); // remove session
  user.set("stime",   Json((json_int_t)time(NULL)));
  udb.put_json(id, user); // Write user to the database

  throw Exc() << user.save_string(JSON_PRESERVE_ORDER);
}

/********************************************************************/
void
do_set_alias(const CFG & cfg, int argc, char **argv){

  Err("set_alias");
  check_args(argc, 1);
  const char *alias = argv[1];

  /* Get user information. Empty session is an error */
  UserDB udb(cfg);
  std::string id;
  Json user = udb.get_by_session(get_secret(), id);
  clr_secret();
  if (!user) throw Err() << "authentication error";

  user.set("alias", Json(alias)); // change alias
  udb.put_json(id,  Json(user)); // Write user to the database

  throw Exc() << user.save_string(JSON_PRESERVE_ORDER);
}

/********************************************************************/
