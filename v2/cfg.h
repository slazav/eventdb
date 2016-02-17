#ifndef CFG_H
#define CFG_H

#include <string>
#include <map>
#include "err.h"
#include <jansson.h>

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

  /* Get string from json object.
     If def=NULL then the non-exist field causes error. */
  std::string get_string(json_t *root,
                         const char *key,
                         const char *def = NULL) const{
    json_t *a = json_object_get(root, key);

    if (!a){ /* no such key */
      if (!def){
        json_decref(root);
        throw Err("read_config")
          << "Can't find parameter: " << key;
      }
      return std::string(def);
    }

    if (!json_is_string(a)){ /* not a string */
      json_decref(root);
      throw Err("read_config")
        << "Can't read string parameter: " << key;
    }
    return std::string(json_string_value(a));
  }

  /* constructor: read config file and set all fields */
  CFG(){
    json_error_t e;
    json_t *root = json_load_file(CFG_FILE, 0, &e);
    if (!root) throw Err("read_config") << e.text;

    if (!json_is_object(root)){
      json_decref(root);
      throw Err("read_config")
        << "Json object expected in " << CFG_FILE;
    }
    datadir = get_string(root, "datadir");
    logsdir = get_string(root, "logsdir");
    filedir = get_string(root, "filedir");
    loginza_id  = get_string(root, "loginza_id");
    loginza_sec = get_string(root, "loginza_sec");

    /* test_users array */
    json_t *tu = json_object_get(root, "test_users");
    if (!tu){
      json_decref(root);
      return;
    }

    if (!json_is_object(tu)){
      json_decref(root);
      throw Err("read_config") << "test_users is not an object";
    }

    const char *key;
    json_t *value;
    json_object_foreach(tu, key, value){
      char *str = json_dumps(value, 0);
      test_users[key] = std::string(str);
      free(str);
    }

    json_decref(root);
  }
};

#endif
