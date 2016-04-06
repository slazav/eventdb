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

#define LEVEL_ANON  -1
#define LEVEL_NORM   0
#define LEVEL_MODER  1
#define LEVEL_ADMIN  2
#define LEVEL_SUPER  3

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
// (not good to use string for secrets)
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
string make_session(){
  string session(25, ' ');
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
//  identity -> (identity, name, alias, session, stime)
//  secondary: sessions aliases

class UserDB : public JsonDB{
  public:
  UserDB(const CFG & cfg, const int flags=0):
       JsonDB(cfg.datadir + "/user", false, flags){
    open_sec("session", false);
    open_sec("alias", false);
  }
  Json get_by_session(const string & sess){
    Json ret = get_sec("session", sess);
    if (ret.size()!=1) return Json::null();
    Json::iterator i = ret.begin();
    return i.val();
  }
  bool alias_exists(const string & alias){
    return get_sec("alias", alias).size()!=0;
  }
  void write(const Json & user){
    put_json(user["identity"].as_string(), user);
  }
};

Json
get_anon(){
  Json ret = Json::object();
  ret.set("identity", "anon");
  ret.set("alias",    "anon");
  ret.set("level",    LEVEL_ANON);
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
    userl.set("level", userd["level"].as_integer());
    userl.set("alias", userd["alias"].as_string());
  }
  else{
    /* new user: for the very first user level="admin" */
    userl.set("level", udb.is_empty()? LEVEL_SUPER:LEVEL_NORM);
  }

  /* Create new session */
  userl.set("session", Json(make_session()));
  userl.set("stime",   Json((json_int_t)time(NULL)));

  /* Write user to the database */
  udb.write(userl);

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

  UserDB udb(cfg, DB_RDONLY);
  Json user = udb.get_by_session(sess);
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
  Json user = udb.get_by_session(get_secret());
  clr_secret();
  if (!user) throw Err() << "authentication error";

  user.del("session"); // remove session
  user.set("stime",   Json((json_int_t)time(NULL)));
  udb.write(user);

  throw Exc() << get_anon().save_string(JSON_PRESERVE_ORDER);
}

/********************************************************************/
void
do_set_alias(const CFG & cfg, int argc, char **argv){

  Err("set_alias");
  check_args(argc, 1);
  const char *alias = argv[1];

  // check symbols in the new alias:
  #define MINALIAS 2
  #define MAXALIAS 20
  const char *accept =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789_";
  // @ is not here, but it exists in automatically created aliases
  int n=strlen(alias);
  if (n<MINALIAS) throw Err() << "too short alias";
  if (n>MAXALIAS) throw Err() << "too long alias";
  if (strspn(alias, accept)!=n)
    throw Err() << "only letters, numbers and _ are allowed in alias";

  /* Get user information. Empty session is an error */
  UserDB udb(cfg);
  Json user = udb.get_by_session(get_secret());
  clr_secret();
  if (!user) throw Err() << "authentication error";

  // check the new alias: does it exists?
  if (udb.alias_exists(alias)) throw Err() << "alias exists";

  user.set("alias", Json(alias)); // change alias
  udb.write(user);

  throw Exc() << user.save_string(JSON_PRESERVE_ORDER);
}

/********************************************************************/
void
do_set_level(const CFG & cfg, int argc, char **argv){

  Err("set_level");
  check_args(argc, 2);
  const char *id2  = argv[1];
  int level = atoi(argv[2]);

  /* Get user information. Empty session is an error */
  UserDB udb(cfg);
  Json user = udb.get_by_session(get_secret());
  clr_secret();
  if (!user) throw Err() << "authentication error";

  /* user which we want to change */
  Json user2 = udb.get_json(id2);
  if (!user2) throw Err() << "no such user";

  // check permissions
  if (user["level"].as_integer() <= user2["level"].as_integer() ||
      user["level"].as_integer() <= level ||
      level < LEVEL_ANON ||
      level > LEVEL_ADMIN) throw Err() << "bad level value";

  user2.set("level", Json(level)); // change alias
  udb.write(user2);

  if (user2.exists("session")) user2.del("session");
  if (user2.exists("stime"))   user2.del("stime");

  throw Exc() << user2.save_string(JSON_PRESERVE_ORDER);
}

/********************************************************************/

void
do_user_list(const CFG & cfg, int argc, char **argv){

  Err("user_list");
  check_args(argc, 0);

  /* Get user information. Empty session is an error */
  UserDB udb(cfg, DB_RDONLY);
  Json user = udb.get_by_session(get_secret());
  clr_secret();
  if (!user) throw Err() << "authentication error";

  // check user level
  int level = user["level"].as_integer();
  if (level<LEVEL_NORM) throw Err() << "user level is too low";

  // all the user database
  Json ret = udb.get_all();

  for (Json::iterator i=ret.begin(); i!=ret.end(); ++i){
    // remove session information
    if (i.val().exists("session")) i.val().del("session");
    if (i.val().exists("stime"))   i.val().del("stime");

    // add level_hints: how can I change the level
    int ln = (i.val())["level"].as_integer();
    if (level>LEVEL_NORM && ln<level){
      Json level_hints = Json::array();
      for (int j=LEVEL_ANON; j<level; j++)
        level_hints.append(Json(j));
      i.val().set("level_hints", level_hints);
    }
  }

  throw Exc() << ret.save_string(JSON_PRESERVE_ORDER);
}
