#include <string>
#include <jansson.h>
#include "err.h"

/*  See json.h */

using namespace std;

/* create json object */
json_t *
j_mkobj(){
  json_t * root = json_object();
  if (!root) throw Err("json_mkobj") << "can't create json object";
}

/* dump json to string */
string
j_dumpstr(json_t *root){
  char * str = json_dumps(root, 0);
  if (!str) throw Err("json_dumpstr") << "can't write json to string";
  string s(str);
  free(str);
  return s;
}

/* load json from string */
json_t *
j_loadstr(const string & str){
  json_error_t e;
  json_t * root = json_loads(str.c_str(), 0, &e);
  if (!root) throw Err("json_loadstr") << e.text;
}

/* load json from file */
json_t *
j_loadfile(const string & file){
  json_error_t e;
  json_t * root = json_load_file(file.c_str(), 0, &e);
  if (!root) throw Err("json_loadfile") << e.text;
}


/* Check that root is an object, get one field. */
/* If def=NULL then the non-exist field causes error.*/
string
j_getstr(json_t *root, const string & key, const char *def = NULL){
  /* check if root is an object object, throw exception if not */
  if (!json_is_object(root)){
    json_decref(root);
    throw Err("json_getstr") << "json object expected";
  }
  /* extract a field */
  json_t * val = json_object_get(root, key.c_str());

  if (!val){ /* no such key */
    if (def) return string(def);
    json_decref(root);
    throw Err("json_getstr")  << "Can't find parameter: " << key;
  }
  if (!json_is_string(val)){
    json_decref(root);
    throw Err("json_getstr") << "json string expected: " << key;
  }
  return string(json_string_value(val));
}

/* put key:pair value into json object */
void
j_putstr(json_t *root, const string & key, const string & val){
  int ret = json_object_set_new(root, key.c_str(), json_string(val.c_str()));
  if (ret) throw Err("json_putstr")  << "Can't put to json: " 
             << key << " : " << val;
}

