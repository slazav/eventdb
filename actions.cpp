#include <iostream>
#include <fstream>

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

//#define JSON_OUT_FLAGS  (JSON_PRESERVE_ORDER | JSON_INDENT(2))
#define JSON_OUT_FLAGS  JSON_PRESERVE_ORDER


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

// get anonimous user
Json
get_anon(){
  Json ret = Json::object();
  ret.set("level",    LEVEL_ANON);
  return ret;
}

// check user alias
#define MINALIAS 2
#define MAXALIAS 30
#define ALIASSYMB\
  "abcdefghijklmnopqrstuvwxyz"\
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"\
  "0123456789_"
void
check_alias(const char *alias){
  int n=strlen(alias);
  if (n<MINALIAS) throw Err() << "too short alias";
  if (n>MAXALIAS) throw Err() << "too long alias";
  if (strspn(alias, ALIASSYMB)!=n)
    throw Err() << "only letters, numbers and _ are allowed in alias";
}

/********************************************************************/
// user database
//  int id -> {faces: [ {id, name, site} ] // login information
//             name, alias, level,        // local information
//             session, stime}            // session information
//  secondary: sessions aliases faces
/********************************************************************/

class UserDB : public JsonDB{
  public:
  UserDB(const CFG & cfg, const int flags=0):
       JsonDB(cfg.datadir + "/user", true, flags){
    secondary_open("session", false);
    secondary_open("alias", false);
    secondary_open("faces", false);
  }
  Json get_by_session(const string & s){
    return secondary_get("session", s)[(size_t)0];
  }
  Json get_by_face(const string & s){
    return secondary_get("faces", s)[(size_t)0];
  }
  Json get_by_alias(const string & s){
    return secondary_get("alias", s)[(size_t)0];
  }
};


/********************************************************************/
// Actions
/********************************************************************/
void
do_login(const CFG & cfg, int argc, char **argv){

  Err("login");                // set error type
  check_args(argc, 0);         // check argument number

  /* get face information (from loginza of cfg file) */
  Json face = get_login_info(cfg, get_secret());
  clr_secret();

  /* extract user identity */
  string face_id = face["id"].as_string();
  if (face_id=="") throw Err() << "login error";

  /* Read user from database or create new one */
  UserDB udb(cfg, DB_CREATE);
  Json user=Json::object();

  if (udb.secondary_exists("faces", face_id)){
    /* known user: find the user with known face id */
    user = udb.get_by_face(face_id);

    /* replace the face (in case we have a changed name) */
    for (int i=0; i<user["faces"].size(); i++){
      if (user["faces"][i]["id"].as_string() != face_id) continue;
      user["faces"].set(i, face);
      user.set("sface", i);
    }
  }
  else{ /* new user */
    user.set("id", -1); // negative id if we want to add new one
    user.set("faces", Json::array());
    user["faces"].append(face);
    user.set("sface", 0);

    // auto level: for the very first user level="admin"
    user.set("level", udb.is_empty()? LEVEL_SUPER:LEVEL_NORM);

    // auto alias:
    // first try an alias, derived from the name -- TODO!
    string name = face["name"].as_string();
    string alias;
    for (int i=0; i<name.length(); i++){
      if (index(ALIASSYMB, name[i])==NULL) continue;
      if (i>=MAXALIAS) break;
      alias.push_back(name[i]);
    }
    if (alias.length() < MINALIAS) alias="user01";

    // if alias exists try "user01", "user02", etc.
    int num=1;
    char nstr[5];
    while (udb.secondary_exists("alias", alias)){
      if (num>=100) throw Err() << "too large number in auto alias";
      snprintf(nstr, sizeof(nstr), "%02d", num++);
      alias = string("user") + nstr;
    }
    user.set("alias", alias);
  }
  /* Create new session */
  user.set("stime",   (json_int_t)time(NULL));
  user.set("session", make_session());

//  std::cerr << "> " <<
//    user.save_string(JSON_PRESERVE_ORDER | JSON_INDENT(2)) << "\n";

  /* Write user to the database */
  udb.put(user);

  /* return user information */
  throw Exc() << user.save_string(JSON_OUT_FLAGS);
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
  if (strlen(sess)==0) // no session
    throw Exc() << get_anon().save_string(JSON_OUT_FLAGS);

  try {
    UserDB udb(cfg, DB_RDONLY);
    Json user = udb.get_by_session(sess);
    clr_secret();

    if (!user) // expired session
      throw Exc() << get_anon().save_string(JSON_OUT_FLAGS);

    throw Exc() << user.save_string(JSON_OUT_FLAGS);
  }
  catch(JsonDB::Err e){ // in case of DB error we also return an anon user.
    throw Exc() << get_anon().save_string(JSON_OUT_FLAGS);
  }
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

  user.set("stime",   (json_int_t)time(NULL));
  user.del("session"); // remove session
  udb.put(user);

  throw Exc() << get_anon().save_string(JSON_OUT_FLAGS);
}

/********************************************************************/
void
do_set_alias(const CFG & cfg, int argc, char **argv){

  Err("set_alias");
  check_args(argc, 1);
  const char *alias = argv[1];

  // check length and symbols in the new alias:
  check_alias(alias);

  /* Get user information. Empty session is an error */
  UserDB udb(cfg);
  Json user = udb.get_by_session(get_secret());
  clr_secret();
  if (!user) throw Err() << "authentication error";

  // check the new alias: does it exists?
  if (udb.secondary_exists("alias", alias)) throw Err() << "alias exists";

  user.set("alias", alias); // change alias
  udb.put(user);

  throw Exc() << user.save_string(JSON_OUT_FLAGS);
}

/********************************************************************/
void
do_set_level(const CFG & cfg, int argc, char **argv){

  Err("set_level");
  check_args(argc, 2);
  const char *alias2  = argv[1];
  int level = atoi(argv[2]);

  /* Get user information. Empty session is an error */
  UserDB udb(cfg);
  Json user = udb.get_by_session(get_secret());
  clr_secret();
  if (!user) throw Err() << "authentication error";

  /* user which we want to change */
  Json user2 = udb.get_by_alias(alias2);
  if (!user2) throw Err() << "no such user";

  // check permissions
  if (user["level"].as_integer() <= user2["level"].as_integer() ||
      user["level"].as_integer() <= level ||
      level < LEVEL_ANON ||
      level > LEVEL_ADMIN) throw Err() << "bad level value";

  user2.set("level", level); // change alias
  udb.put(user2);

  if (user2.exists("session")) user2.del("session");
  if (user2.exists("stime"))   user2.del("stime");

  throw Exc() << user2.save_string(JSON_OUT_FLAGS);
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
  if (level<LEVEL_MODER) throw Err() << "user level is too low";

  // all the user database
  Json ret = udb.get_all();

  for (size_t i = 0; i<ret.size(); i++){
    // remove session information
    if (ret[i].exists("session")) ret[i].del("session");
    if (ret[i].exists("stime"))   ret[i].del("stime");
    if (ret[i].exists("sface"))   ret[i].del("sface");

    // add level_hints: how can I change the level
    int ln = ret[i]["level"].as_integer();
    if (level>LEVEL_NORM && ln<level){
      Json level_hints = Json::array();
      for (int j=LEVEL_ANON; j<level; j++)
        level_hints.append(Json(j));
      ret[i].set("level_hints", level_hints);
    }
  }

  throw Exc() << ret.save_string(JSON_OUT_FLAGS);
}

/********************************************************************/
// join requests
void
do_joinreq_add(const CFG & cfg, int argc, char **argv){
  Err("joinreq_add");
  check_args(argc, 1);
  const char *alias2  = argv[1];

  /* Get user information. Empty session is an error */
  UserDB udb(cfg);
  Json user = udb.get_by_session(get_secret());
  clr_secret();
  if (!user) throw Err() << "authentication error";

  /* the second user */
  Json user2 = udb.get_by_alias(alias2);
  if (!user2) throw Err() << "no such user";

  // add all faces to the request
  user2.set("joinreq", user["faces"]);
  udb.put(user2);
}

void
do_joinreq_delete(const CFG & cfg, int argc, char **argv){
  Err("joinreq_delete");
  check_args(argc, 1);
  size_t num = atoi(argv[1]);

  /* Get user information. Empty session is an error */
  UserDB udb(cfg);
  Json user = udb.get_by_session(get_secret());
  clr_secret();
  if (!user) throw Err() << "authentication error";

  user["joinreq"].del(num);
  if (user["joinreq"].size()==0) user.del("joinreq");
  udb.put(user);

  throw Exc() << user.save_string(JSON_OUT_FLAGS);
}

void
do_joinreq_accept(const CFG & cfg, int argc, char **argv){
  Err("joinreq_accept");
  check_args(argc, 1);
  size_t num = atoi(argv[1]);

  /* Get user information. Empty session is an error */
  UserDB udb(cfg);
  Json user = udb.get_by_session(get_secret());
  clr_secret();
  if (!user) throw Err() << "authentication error";

  string id2 = user["joinreq"][num]["id"].as_string();

  /* the second user */
  Json user2 = udb.get_by_face(id2);
  if (!user2) throw Err() << "no such user";

  /* set the level to the maximum */
  int l1 = user["level"].as_integer();
  int l2 = user2["level"].as_integer();
  user.set("level", max(l1,l2));

  /* Remove the face. If needed, remove the user */
  for (size_t i=0; i<user2["faces"].size(); i++){
     if (user2["faces"][i]["id"].as_string() == id2)
     user2["faces"].del(i);
  }
  if (user2["faces"].size()==0) udb.del(user2["id"].as_integer());
  else udb.put(user2);

  /* Put the face into the first user */
  user["faces"].append(user["joinreq"][num]);
  user["joinreq"].del(num);
  if (user["joinreq"].size()==0) user.del("joinreq");
  udb.put(user);

  throw Exc() << user.save_string(JSON_OUT_FLAGS);
}

/********************************************************************/
// dump databases -- can not be called from www interface
void
local_dump_db(const CFG & cfg, int argc, char **argv){

  /* Get user information. Empty session is an error */
  UserDB udb(cfg);

  const char *user_keys[] = {"", "session", "alias", "faces", NULL};
  for (const char **c = user_keys; *c!=NULL; c++){
    string fname = udb.get_fname() + (strlen(*c)?".":"") + *c + ".txt";
    ofstream out(fname.c_str());
    out << udb.get_all(*c).save_string(JSON_INDENT(2) | JSON_PRESERVE_ORDER);
  }

}


/********************************************************************/
// event database
//  id -> {id, name, text, date1, date2,
//         [people], [links], [tags]
//         cuser, muser, ctime, mtime} // db information
//  secondary: date1, date2
//  people: array of strings ?
//  tags:   array of strings ?
//  links:  array of objects {ref, text, type, auth, tags}
/********************************************************************/

class EventDB : public JsonDB{
  public:
  EventDB(const CFG & cfg, const int flags=0):
       JsonDB(cfg.datadir + "/event", true, flags){
    secondary_open("date1",  true);
    secondary_open("date2",  true);
    secondary_open("tags",   true);
    secondary_open("people", true);
  }
};

// check date string YYYY-MM-DD
bool check_date(const char * d){
  if (strlen(d) != 10) return false;
  for (int i=0; i<strlen(d); i++){
    if ((i==5 || i==8) && d[i]!='-') return false;
    if (i!=5 && i!=8 && (d[i]<'0' || d[i]>'9')) return false;
  }
  return true;
}

/********************************************************************/
// Event-related actions
/********************************************************************/
void
do_ev_new(const CFG & cfg, int argc, char **argv){
  Err("ev_new");
  check_args(argc, 1);
  Json ev = Json::load_file(argv[1]);
  if (!ev) throw Err() << "bad json input";

  /* Get user information. Empty session is an error */
  UserDB udb(cfg, DB_RDONLY);
  Json user = udb.get_by_session(get_secret());
  clr_secret();
  if (!user) throw Err() << "authentication error";

  // check user level
  int level = user["level"].as_integer();
  if (level<LEVEL_NORM) throw Err() << "user level is too low";

  // check dates
  std::string d1 = ev["date1"].as_string();
  std::string d2 = ev["date2"].as_string();
  if (d2 == "") d2=d1;
  if (!check_date(d1.c_str())) throw Err() << "date1 is not in YYYY-MM-DD format";
  if (!check_date(d2.c_str())) throw Err() << "date2 is not in YYYY-MM-DD format";

  // check title


}


/********************************************************************/
void
do_ev_del(const CFG & cfg, int argc, char **argv){
}

/********************************************************************/
void
do_ev_edit(const CFG & cfg, int argc, char **argv){
}

/********************************************************************/
void
do_ev_show(const CFG & cfg, int argc, char **argv){
}

/********************************************************************/
void
do_ev_list(const CFG & cfg, int argc, char **argv){
}

/********************************************************************/
