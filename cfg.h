#ifndef CFG_H
#define CFG_H

#include <string>
#include <map>
#include "err.h"
#include "jsonxx/jsonxx.h"

/*
  Read config file CFG_FILE (json format).
  Keep variables:
    datadir      string
    logsdir      string
    filedir      string
    loginza_id   string
    loginza_sec  string
    test_users   map(key,json)
*/


class CFG{
public:

  std::string datadir, logsdir, filedir;
  std::string loginza_id, loginza_sec;
  std::map<std::string, std::string> test_users;

  /* constructor: read config file and set all fields */
  CFG(const char * cfg_file){
    Err("read_config");
    Json root = Json::load_file(cfg_file);
    datadir = root["datadir"].as_string();
    logsdir = root["logsdir"].as_string();
    filedir = root["filedir"].as_string();
    loginza_id  = root["loginza_id"].as_string();
    loginza_sec = root["loginza_sec"].as_string();

    /* test_users object */
    Json uu = root["test_users"];
    for (Json::iterator i = uu.begin(); i!=uu.end(); i++)
      test_users[i.key()] = i.val().save_string(JSON_PRESERVE_ORDER);
  }
};

#endif
