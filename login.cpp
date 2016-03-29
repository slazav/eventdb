#include <iostream>
#include <fstream>

#include <cstdio> /* sprintf */
#include <cstring> /* memcpy, memset */
#include <unistd.h> /* read, write, close */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <openssl/md5.h>
#include "login.h"

using namespace std;

/* See login.h for the interface description. */

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
    if (sockfd < 0) throw Err() << "can't open a socket";

    /* lookup the ip address */
    server = gethostbyname(host);
    if (server == NULL) throw Err() << "no such host: " << host;

    /* fill in the structure */
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    /* connect the socket */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
         throw Err() << "connection error";

    /* send the request */
    total = strlen(strbuf);
    sent = 0;
    do {
      bytes = write(sockfd,strbuf+sent,total-sent);
      if (bytes < 0) throw Err() << "can't write message to a socket";
      if (bytes == 0) break;
      sent+=bytes;
    } while (sent < total);

    /* receive the response */
    memset(strbuf,0,sizeof(strbuf));
    total = sizeof(strbuf)-1;
    received = 0;
    do {
        bytes = read(sockfd,strbuf+received,total-received);
        if (bytes < 0) throw Err() << "bad reading response from socket";
        if (bytes == 0) break;
        received+=bytes;
    } while (received < total);

    if (received == total) throw Err() << "too long response from loginza";

    /* close the socket */
    close(sockfd);
  }
  return string(strbuf);
}

/********************************************************************/
/* extract provider from identity */
string get_provider(const string identity){
  if (identity.find("www.facebook.com") != string::npos) return string("fb");
  if (identity.find("www.google.com")   != string::npos) return string("google");
  if (identity.find("openid.yandex.ru") != string::npos) return string("yandex");
  if (identity.find("vk.com")           != string::npos) return string("vk");
  if (identity.find("livejournal.com")  != string::npos) return string("lj");
  return string();
}

/********************************************************************/
/* Convert login information from string to standard json.
 * (original information can be different for different providers).
 */
json::Value
parse_login_data(const string & juser){

  /* convert string to json */
  json::Value root = JsonDB::str2json(juser);

  /* if loginza returned an error - throw it */
  if (root.get("error_message").is_undefined())
    throw Exc() << juser;

  /* get identity field */
  std::string identity = JsonDB::json_getstr(root, "identity");

  /* default full_name and alias */
  std::string full_name, alias;
  full_name = alias = identity;

  /* extract provider */
  std::string provider = get_provider(identity);

  /* parse the name if possible and make correct full name and alias */
  const json::Value nn = root.getv("name");
  if (nn.is_object()){
    string n1 = JsonDB::json_getstr(nn, "first_name", "");
    string n2 = JsonDB::json_getstr(nn, "last_name", "");

    if (n1!="" && n2!="") { full_name = n1+" "+n2; alias = n1+n2; }
    else if (n1!="")  alias = full_name = n1;
    else if (n2!="")  alias = full_name = n2;
    alias += "@" + provider;
  }

  /* lj has only identity */
  if (provider == "lj"){
    size_t i1 = identity.find("http://");
    size_t i2 = identity.find(".livejournal.com");
    full_name = identity.substr(i1+7, i2-i1-7);
    alias     = full_name + "@lj";
  }

  /* build the output json */
  json::Value ret(json::object());
  ret.set_key("identity",  json::Value(identity));
  ret.set_key("provider",  json::Value(provider));
  ret.set_key("full_name", json::Value(full_name));
  ret.set_key("alias",     json::Value(alias));
  return ret;
}

/* string with current date and time */
string
time_str(){
  char tstr[25];
  time_t lt;
  time (&lt);
  struct tm * t = localtime(&lt);
  snprintf(tstr, sizeof(tstr), "%04d-%02d-%02d %02d:%02d:%02d",
    t->tm_year+1900, t->tm_mon+1, t->tm_mday,
    t->tm_hour, t->tm_min, t->tm_sec);
  return string(tstr);
}

/* get login data and create standard json object */
json::Value
get_login_info(const CFG & cfg, const char *tok){

  /* get user login information: test_users or loginza */
  string juser;
  if (cfg.test_users.find(tok)!=cfg.test_users.end()){
    juser = cfg.test_users.find(tok)->second;
  }
  else{ /* loginza */
    juser = ask_loginza(tok,
      cfg.loginza_sec.c_str(), cfg.loginza_sec.c_str());
  }

  /* fill token with 0 */
  memset(tok, 0, strlen(tok));

  /* log the information */
  ofstream out((cfg.logsdir + "/login.txt").c_str(), ios::app);
  out << time_str() << " " << juser << std::endl;

  /* parse json string from loginza and fill user information */
  return parse_login_data(juser);
}