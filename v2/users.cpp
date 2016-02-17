#include <iostream>

#include <cstdio> /* sprintf */
#include <cstring> /* memcpy, memset */
#include <unistd.h> /* read, write, close */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <openssl/md5.h>
#include <jansson.h>

#include "act.h"
#include "dbs.h"

using namespace std;

/********************************************************************/
/* ask loginza for user data */
string ask_loginza(const char *tok,
                        const char *id,
                        const char *sec){
  char strbuf[4096];

  /*****************************/
  /* Format the request string */
  {
    uint8_t md5[MD5_DIGEST_LENGTH];
    char md5s[2*MD5_DIGEST_LENGTH+1];

    /* create md5 of the token+secret */
    snprintf(strbuf,sizeof(strbuf), "%s%s", tok, sec);
    MD5((unsigned char *)strbuf, strlen(strbuf),md5);

    /* copy md5 in the md5s string */
    for (int i=0; i<sizeof(md5); i++) sprintf(md5s+2*i, "%02x", md5[i]);

    /* fill in the parameters */
    snprintf(strbuf,sizeof(strbuf),
      "GET /api/authinfo?token=%s&id=%s&sig=%s\r\n",
      tok, id, md5s);
  }
  /*****************************/
  /* Open connection to the loginza server, send request from strbuf,
     get response to strbuf. */
  {
    const int portno = 80;
    const char *host = "loginza.ru";
    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total;

    /* create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) throw Err("loginza_connection") << "can't open a socket";

    /* lookup the ip address */
    server = gethostbyname(host);
    if (server == NULL) throw Err("loginza_connection") << "no such host";

    /* fill in the structure */
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    /* connect the socket */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
         throw Err("loginza_connection") << "connection error";

    /* send the request */
    total = strlen(strbuf);
    sent = 0;
    do {
      bytes = write(sockfd,strbuf+sent,total-sent);
      if (bytes < 0) throw Err("loginza_connection")
          << "can't write message to a socket";
      if (bytes == 0) break;
      sent+=bytes;
    } while (sent < total);

    /* receive the response */
    memset(strbuf,0,sizeof(strbuf));
    total = sizeof(strbuf)-1;
    received = 0;
    do {
        bytes = read(sockfd,strbuf+received,total-received);
        if (bytes < 0) throw Err("loginza_connection")
            << "bad reading response from socket";
        if (bytes == 0) break;
        received+=bytes;
    } while (received < total);

    if (received == total) throw Err("loginza_connection")
          << "too long response";

    /* close the socket */
    close(sockfd);
  }
  return string(strbuf);
}
/********************************************************************/
/* parse user login data */

string get_privider(const string identity){
  /* extract provider */
  if      (identity.find("www.facebook.com") != string::npos) return string("fb");
  else if (identity.find("www.google.com")   != string::npos) return string("google");
  else if (identity.find("openid.yandex.ru") != string::npos) return string("yandex");
  else if (identity.find("vk.com")           != string::npos) return string("vk");
  else if (identity.find("livejournal.com")  != string::npos) return string("lj");
  return string();
}

/********************************************************************/
/** Actions
/********************************************************************/
void
do_login(const CFG & cfg, int argc, char **argv){

  /* check arguments*/
  if (argc!=1) throw Err("login")
    << "wrong number of arguments, should be 0";


  /* read login token from stdin*/
  string token;
  cin >> token;
  if (!cin.good())
    throw Err("eventdb_login") << "login token expected";


  /* get user login information: test_users or loginza */
  string juser;
  if (cfg.test_users.find(token)!=cfg.test_users.end()){
    juser = cfg.test_users.find(token)->second;
  }
  else{ /* loginza */
    juser = ask_loginza(token.c_str(),
      cfg.loginza_sec.c_str(), cfg.loginza_sec.c_str());
  }


  /* Parse user login information. Do not use any strict formats here */
  string identity, // https://www.facebook.com\/app_scoped_user_id\/100004349705357/
         full_name,      // Zavjalov Vladislav
         provider,       // fb, google, lj, vk etc.
         alias,          // Zavjalov Vladislav @fb
         abbr,           // ZV
         level;          // banned, normal, moder, admin

  /* parse json, throw exception on error */
  json_error_t e;
  json_t * root = json_loads(juser.c_str(), 0, &e);
  if (!root) throw Err("user_login_data") << e.text;

  /* check if json_t is an object object, throw exception if not */
  if (!json_is_object(root)){
    json_decref(root);
    throw Err("user_login_data") << "json object expected";
  }

  /* extract identity */
  json_t * id = json_object_get(root, "identity");
  if (!json_is_string(id)){
    json_decref(root);
    throw Err("user_login_data") << "json string expected for identity";
  }

  /* defaults */
  full_name = alias = identity = json_string_value(id);
  level = "normal";

  /* extract provider */
  provider = get_privider(identity);

  /* parse the name if possible */
  json_t *name = json_object_get(root, "name");
  if (json_is_object(name)){
    const char *n1 = json_string_value(json_object_get(name, "first_name"));
    const char *n2 = json_string_value(json_object_get(name, "last_name"));

    if (n1 && n2) full_name = string(n1) + " " + string(n2);
    else if (n1)  full_name = string(n1);
    else if (n2)  full_name = string(n2);
    alias = full_name + " @" + provider;
  }

  /* lj has only identity */
  if (provider == "lj"){
    size_t n1 = identity.find("http://");
    size_t n2 = identity.find(".livejournal.com");
    full_name = identity.substr(n1+7, n2-n1-7);
    alias     = full_name + "@lj";
  }
  json_decref(root);


  /* Now we know the identity. Try to get user information from the database */
  Database user_db;
  user_db.open(cfg.datadir + "/users.db", DB_CREATE);

  if (user_db.exists(identity)){ // old user
    /* extract user information from the database: aliases and level */
    root = user_db.get_json(identity);
    const char *c_alias, *c_abbr, *c_level;
    int ret = json_unpack_ex(root, &e, 0, "{s:s, s:s, s:s}",
      "alias", &c_alias, "abbr", &c_abbr, "level", &c_level);
    if (ret){
      json_decref(root);
      throw Err("read_user_data") << e.text;
    }
    alias = c_alias; abbr = c_abbr; level = c_level;
    json_decref(root);
  }
  else { // new user
    /* put the new user into the database */

    // TODO: check if level=100 is needed

    // TODO: check that alias is unique!

    // put entry into the database
    root = json_pack_ex(&e, 0, "{ssssss}",
      "alias", alias.c_str(), "abbr", abbr.c_str(), "level", level.c_str());
    if (!root){
      json_decref(root);
      throw Err("create_user") << e.text;
    }
    user_db.put_json(identity, root, false);
    json_decref(root);
  }
  user_db.close();

  // TODO: remove old sessions

  // create a session
  std::string session(25, ' ');
  static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (int i = 0; i < session.length(); ++i)
    session[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

  for (int i=0; i<20; i++)
  root = json_pack_ex(&e, 0, "{ssss}",
    "id", identity.c_str(), "name", full_name.c_str());
  json_decref(root);

  /* TODO: return session information */

  throw Exc() << "id: <" << identity << ">";
}

void
do_logout(const CFG & cfg, int argc, char **argv){
  if (argc!=1) throw Err("login")
    << "wrong number of arguments, should be 0";
}
