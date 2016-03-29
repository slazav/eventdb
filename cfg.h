#ifndef CFG_H
#define CFG_H

#include <string>
#include <map>
#include "err.h"
#include "jsondb/jsondb.hpp"

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
    json::Value root = JsonDB::file2json(cfg_file);
    datadir = JsonDB::json_getstr(root, "datadir");
    logsdir = JsonDB::json_getstr(root, "logsdir");
    filedir = JsonDB::json_getstr(root, "filedir");
    loginza_id  = JsonDB::json_getstr(root, "loginza_id");
    loginza_sec = JsonDB::json_getstr(root, "loginza_sec");

    /* test_users object */
    json::Iterator i(root["test_users"]);
    while (i.valid()) {
      test_users[i.key()] = i.value().save_string();
      i.next();
    }
  }
};

#endif
