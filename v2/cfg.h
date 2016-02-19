#ifndef CFG_H
#define CFG_H

#include <string>
#include <map>
#include "err.h"
#include "json.h"

#ifndef CFG_FILE
#define CFG_FILE "./config.json"
#endif

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
  CFG(){
    Err("read_config");
    json_t *root = j_loadfile(CFG_FILE);
    datadir = j_getstr(root, "datadir");
    logsdir = j_getstr(root, "logsdir");
    filedir = j_getstr(root, "filedir");
    loginza_id  = j_getstr(root, "loginza_id");
    loginza_sec = j_getstr(root, "loginza_sec");

    /* test_users array */
    json_t *tu = json_object_get(root, "test_users");
    if (!tu){
      json_decref(root);
      return;
    }
    if (!json_is_object(tu)){
      json_decref(root);
      throw Err() << "test_users is not an object";
    }
    const char *key;
    json_t *value;
    json_object_foreach(tu, key, value){
      test_users[key] = j_dumpstr(value);
    }
    json_decref(root);
  }
};

#endif
